import socket, time, sys

host = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
port = int(sys.argv[2]) if len(sys.argv) > 2 else 9876

s = socket.socket()
s.settimeout(10)
s.connect((host, port))
print("connected", flush=True)
s.sendall(b'{"id":"t1","action":"ping","payload":{}}\n')
print("sent", flush=True)
buf = b""
for i in range(30):
    try:
        chunk = s.recv(4096)
        if chunk:
            buf += chunk
            print(f"  recv[{i}]: {chunk!r}", flush=True)
            if b"\n" in buf:
                break
    except socket.timeout:
        print(f"  timeout[{i}]", flush=True)
    time.sleep(0.5)
print(f"final buffer: {buf!r}", flush=True)
s.close()
