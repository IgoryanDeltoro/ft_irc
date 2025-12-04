# Sends a command split into parts to test partial aggregation
import socket
import time


HOST = '127.0.0.1'
PORT = 6767
PASS = 'mypass'


s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST, PORT))
s.getsockname()
# send PASS in fragments
s.sendall(b'PA')
time.sleep(0.05)
s.sendall(b'SS ' + PASS.encode() + b'\r\n')
time.sleep(0.05)
s.sendall(b'')
# similarly send NICK in fragments
s.sendall(b'NI')
time.sleep(0.05)
s.sendall(b'CK fraguser\r\n')
# USER
s.sendall(b'USER fraguser 0 * :Partial\r\n')
# collect
time.sleep(0.2)
s.settimeout(1.0)
data = b''
try:
    while True:
        chunk = s.recv(4096)
        if not chunk: break
        data += chunk
except Exception:
    pass
print('RECV (partial test): ', data.decode(errors='ignore'))
s.close()