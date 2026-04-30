import json
import logging
import socket
import uuid

# 最大响应匹配尝试次数，防止串包时无限循环
_MAX_RESPONSE_MATCH_ATTEMPTS = 32

logger = logging.getLogger(__name__)


class UEBridgeError(ConnectionError):
    """UE 桥接连接错误基类，携带错误分类码"""
    def __init__(self, code, message):
        super().__init__(message)
        self.code = code          # 错误分类码（CONNECT_TIMEOUT / READ_TIMEOUT / PEER_CLOSED 等）


class UEBridgeClient:
    """
    UE 桥接 TCP 客户端
    通过 JSON Lines 协议与 Unreal Editor 中的 MCPBridgeServer 通信
    支持自动重连与连接断开检测，响应包含 ID 校验防止串包
    注意：UE 侧为单客户端独占模型，同一时刻只允许一个连接
    """

    def __init__(self, host="127.0.0.1", port=9876, timeout=5.0, token=None):
        self._host = host          # UE 桥接服务地址（默认本机回环）
        self._port = port          # UE 桥接服务端口
        self._timeout = timeout    # socket 操作超时秒数
        self._sock = None          # TCP socket 实例
        self._buffer = ""          # 接收缓冲区（累积未完成的 JSON 行）
        self._token = token        # 可选共享令牌（用于写/危险操作鉴权）

    def connect(self):
        """建立 TCP 连接，清空缓冲区；连接失败时抛出 UEBridgeError"""
        try:
            self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self._sock.settimeout(self._timeout)
            self._sock.connect((self._host, self._port))
            self._buffer = ""
            logger.debug("Connected to UE bridge at %s:%d", self._host, self._port)
        except socket.timeout:
            self._sock = None
            raise UEBridgeError(
                "CONNECT_TIMEOUT",
                f"UE bridge connect timeout ({self._timeout}s) to {self._host}:{self._port}. "
                "Ensure Unreal Editor is running with UnrealEditorMCPBridge plugin loaded."
            )
        except ConnectionRefusedError:
            self._sock = None
            raise UEBridgeError(
                "CONNECT_REFUSED",
                f"UE bridge connection refused at {self._host}:{self._port}. "
                "Ensure Unreal Editor is running and the bridge server has started."
            )
        except OSError as e:
            self._sock = None
            raise UEBridgeError("CONNECT_FAILED", f"UE bridge connect failed: {e}")

    def disconnect(self):
        """安全关闭连接，忽略关闭时的异常"""
        if self._sock:
            try:
                self._sock.close()
            except Exception:
                pass
            self._sock = None
            logger.debug("Disconnected from UE bridge")

    def _ensure_connected(self):
        """检查连接状态，未连接时自动建立"""
        if self._sock is None:
            self.connect()

    def send(self, action, payload=None):
        """
        发送请求并等待匹配的响应（按 id 校验）
        action: UE 侧动作名（ping / get_editor_info 等，不含 ue_ 前缀）
        payload: 请求参数字典，可选
        返回: 解析后的 JSON 响应字典 { id, ok, result, error }
        抛出: UEBridgeError（带错误分类码）
        """
        request_id = str(uuid.uuid4())   # 自动生成唯一请求 ID
        payload = payload or {}
        # 注入 token（若已配置）
        if self._token:
            payload["_token"] = self._token
        request = {
            "id": request_id,
            "action": action,
            "payload": payload,
        }
        try:
            self._ensure_connected()
            # JSON Lines 格式：JSON + 换行
            data = json.dumps(request) + "\n"
            self._sock.sendall(data.encode("utf-8"))
            return self._read_response(request_id)
        except UEBridgeError:
            # 已分类的错误直接上抛，不做二次包装
            self.disconnect()
            raise
        except (socket.timeout, OSError) as e:
            self.disconnect()
            raise UEBridgeError("CONNECTION_LOST", f"UE bridge connection lost: {e}")

    def _read_response(self, expected_id):
        """
        阻塞读取直到收到 id 匹配的完整 JSON 响应
        跳过不匹配的行，最多尝试 _MAX_RESPONSE_MATCH_ATTEMPTS 次
        抛出: UEBridgeError（READ_TIMEOUT / PEER_CLOSED / RESPONSE_MISMATCH / CLIENT_ALREADY_CONNECTED）
        """
        for attempt in range(_MAX_RESPONSE_MATCH_ATTEMPTS):
            while "\n" not in self._buffer:
                try:
                    data = self._sock.recv(4096)
                except socket.timeout:
                    raise UEBridgeError(
                        "READ_TIMEOUT",
                        f"UE bridge read timeout ({self._timeout}s) after {attempt} lines read. "
                        "The UE Editor may be busy or unresponsive."
                    )
                if not data:
                    raise UEBridgeError(
                        "PEER_CLOSED",
                        "UE bridge connection closed by peer (UE Editor may have shut down or the server stopped)."
                    )
                self._buffer += data.decode("utf-8")

            # 取第一行完整 JSON，剩余数据保留在缓冲区
            line, _, rest = self._buffer.partition("\n")
            self._buffer = rest
            resp = json.loads(line)

            # 检查是否被 UE 侧拒绝（如单客户端独占错误）
            if not resp.get("ok"):
                error = resp.get("error", {})
                error_code = error.get("code", "") if isinstance(error, dict) else ""
                if error_code == "CLIENT_ALREADY_CONNECTED":
                    raise UEBridgeError(
                        "CLIENT_ALREADY_CONNECTED",
                        f"UE bridge rejected connection: {error.get('message', 'Client already connected')}"
                    )

            # ID 校验：匹配则返回，不匹配则跳过继续读下一行
            if resp.get("id") == expected_id:
                return resp

            logger.debug("Response ID mismatch: expected %s, got %s (skipping)", expected_id, resp.get("id"))

        raise UEBridgeError(
            "RESPONSE_MISMATCH",
            f"Response ID mismatch after {_MAX_RESPONSE_MATCH_ATTEMPTS} attempts: expected {expected_id}"
        )

    def is_connected(self):
        """返回当前是否有活跃连接"""
        return self._sock is not None
