import socket,json

class SocketServer:
    def __init__(self,host="127.0.0.1",port=5000):
        s=socket.socket(); s.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
        s.bind((host,port)); s.listen(1); print("Agent waiting for communication...")
        self.c,_=s.accept(); s.close(); self.f=self.c.makefile()

    def send_json(self,j):
        self.c.sendall((json.dumps(j)+"\n").encode())

    def receive_json(self):
        x=self.f.readline()
        return json.loads(x) if x else None

    def close(self):
        self.f.close(); self.c.close()


server: SocketServer | None = None


def get_server() -> SocketServer:
    global server

    if server is None:
        server = SocketServer()

    return server
