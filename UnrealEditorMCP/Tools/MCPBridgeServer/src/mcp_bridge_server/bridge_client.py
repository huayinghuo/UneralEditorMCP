import json
import socket
import uuid

# 最大响应匹配尝试次数，防止串包时无限循环
_MAX_RESPONSE_MATCH_ATTEMPTS = 32


class UEBridgeClient:
    """
    UE 桥接 TCP 客户端
    通过 JSON Lines 协议与 Unreal Editor 中的 MCPBridgeServer 通信
    支持自动重连与连接断开检测，响应包含 ID 校验防止串包
    """

    def __init__(self, host="127.0.0.1", port=9876, timeout=5.0, token=None):
        self._host = host          # UE 桥接服务地址（默认本机回环）
        self._port = port          # UE 桥接服务端口
        self._timeout = timeout    # socket 操作超时秒数
        self._sock = None          # TCP socket 实例
        self._buffer = ""          # 接收缓冲区（累积未完成的 JSON 行）
        self._token = token        # 可选共享令牌（用于写/危险操作鉴权）

    def connect(self):
        """建立 TCP 连接，清空缓冲区"""
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.settimeout(self._timeout)
        self._sock.connect((self._host, self._port))
        self._buffer = ""

    def disconnect(self):
        """安全关闭连接，忽略关闭时的异常"""
        if self._sock:
            try:
                self._sock.close()
            except Exception:
                pass
            self._sock = None

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
        except (socket.timeout, ConnectionError, OSError) as e:
            # 连接异常时断开，下次调用自动重连
            self.disconnect()
            raise ConnectionError(f"UE bridge connection failed: {e}")

    def _read_response(self, expected_id):
        """阻塞读取直到收到 id 匹配的完整 JSON 响应（跳过不匹配的行，最多尝试 _MAX_RESPONSE_MATCH_ATTEMPTS 次）"""
        for _ in range(_MAX_RESPONSE_MATCH_ATTEMPTS):
            while "\n" not in self._buffer:
                try:
                    data = self._sock.recv(4096)
                except socket.timeout:
                    raise ConnectionError("UE bridge read timeout")
                if not data:
                    raise ConnectionError("UE bridge connection closed by peer")
                self._buffer += data.decode("utf-8")

            # 取第一行完整 JSON，剩余数据保留在缓冲区
            line, _, rest = self._buffer.partition("\n")
            self._buffer = rest
            resp = json.loads(line)

            # ID 校验：匹配则返回，不匹配则跳过继续读下一行
            if resp.get("id") == expected_id:
                return resp

        raise ConnectionError(
            f"Response ID mismatch after {_MAX_RESPONSE_MATCH_ATTEMPTS} attempts: expected {expected_id}"
        )

    def is_connected(self):
        """返回当前是否有活跃连接"""
        return self._sock is not None
