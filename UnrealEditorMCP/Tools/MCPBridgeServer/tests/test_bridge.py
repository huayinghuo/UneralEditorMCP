import json
import socket
import sys
import time
import uuid


def send_request(sock, action, payload=None, timeout=5.0):
    request = {
        "id": str(uuid.uuid4())[:8],
        "action": action,
        "payload": payload or {},
    }
    data = json.dumps(request) + "\n"
    sock.sendall(data.encode("utf-8"))
    sock.settimeout(timeout)
    buf = b""
    while b"\n" not in buf:
        try:
            chunk = sock.recv(4096)
        except socket.timeout:
            print(f"  TIMEOUT waiting for response")
            return None
        if not chunk:
            print(f"  Connection closed by peer")
            return None
        buf += chunk
    line, _, rest = buf.partition(b"\n")
    sock.settimeout(timeout)
    return json.loads(line.decode("utf-8"))


def test_action(sock, action, payload=None):
    print(f"  {action}...", end=" ", flush=True)
    try:
        resp = send_request(sock, action, payload)
        if resp is None:
            print("NO RESPONSE")
            return False
        if resp.get("ok"):
            print("OK")
            return True
        else:
            err = resp.get("error", {})
            code = err.get("code", "?") if isinstance(err, dict) else "?"
            msg = err.get("message", str(err)) if isinstance(err, dict) else str(err)
            print(f"ERROR [{code}]: {msg[:80]}")
            return False
    except Exception as e:
        print(f"EXCEPTION: {e}")
        return False


def main():
    host = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 9876

    print(f"Connecting to {host}:{port}...", end=" ", flush=True)
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(5.0)

    start = time.time()
    while time.time() - start < 60:
        try:
            sock.connect((host, port))
            print("CONNECTED")
            time.sleep(0.5)
            break
        except (ConnectionRefusedError, OSError):
            time.sleep(1.0)
    else:
        print("TIMEOUT - UE Editor not responding on port")
        sock.close()
        return 1

    results = []
    results.append(test_action(sock, "ping"))
    results.append(test_action(sock, "get_editor_info"))
    results.append(test_action(sock, "get_project_info"))
    results.append(test_action(sock, "get_selected_actors"))
    results.append(test_action(sock, "list_level_actors"))
    results.append(test_action(sock, "execute_python_snippet", {"code": "print('hello from mcp test')"}))
    results.append(test_action(sock, "execute_python_snippet", {"code": "1/0"}))
    results.append(test_action(sock, "nonexistent_action"))
    results.append(test_action(sock, "get_world_state"))
    results.append(test_action(sock, "level_get_actor_property", {"actor_name": "NonExistentActorXYZ", "property_name": "bHidden"}))

    sock.close()

    passed = sum(1 for r in results if r)
    total = len(results)
    print(f"\nResults: {passed}/{total} passed")
    return 0 if passed == total else 1


if __name__ == "__main__":
    sys.exit(main())
