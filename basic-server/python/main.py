import socket
import os

class HTTPServer:
    def __init__(self, host='127.0.0.1', port=8008, web_dir='../www'):
        self.host = host
        self.port = port
        self.web_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), web_dir))

    def start(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.bind((self.host, self.port))
            s.listen()
            print(f"Listening on {self.host}:{self.port}")
            while True:
                conn, addr = s.accept()
                with conn:
                    print('Connected by', addr)
                    request_data = conn.recv(1024)
                    request = HTTPRequest(request_data.decode('utf-8'))
                    
                    if request.method == 'GET':
                        response = self.handle_get_request(request)
                    else:
                        response = HTTPResponse(status_code=405, body="<h1>405 Method Not Allowed</h1>")

                    conn.sendall(response.to_bytes())

    def handle_get_request(self, request):
        path = request.path
        if path == '/':
            path = '/index.html'

        file_path = os.path.join(self.web_dir, path.lstrip('/'))

        if os.path.exists(file_path) and os.path.isfile(file_path):
            with open(file_path, 'r') as f:
                content = f.read()
            return HTTPResponse(status_code=200, body=content)
        else:
            return HTTPResponse(status_code=404, body="<h1>404 Not Found</h1>")


class HTTPRequest:
    def __init__(self, raw_request):
        self.raw = raw_request
        self.method = ""
        self.path = ""
        self.headers = {}
        self.body = ""
        self.parse()

    def parse(self):
        if not self.raw:
            return

        lines = self.raw.split('\r\n')    # RFC 2616 (CLRF, in 2.2: https://www.rfc-editor.org/rfc/rfc2616.html#section-2.2)
        request_line = lines[0]
        self.method, self.path, _ = request_line.split(' ')

        header_lines = lines[1:lines.index('')] if '' in lines else lines[1:]
        for line in header_lines:
            key, value = line.split(': ', 1)
            self.headers[key] = value

        if '' in lines:
            self.body = lines[lines.index('') + 1]

class HTTPResponse:
    def __init__(self, status_code=200, body="", headers=None):
        self.status_code = status_code
        self.body = body
        self.headers = headers if headers is not None else {"Content-Type": "text/html"}

    def to_bytes(self):
        status_line = f"HTTP/1.1 {self.status_code} {self._get_status_message()}\r\n"
        
        self.headers['Content-Length'] = len(self.body)

        header_lines = ""
        for key, value in self.headers.items():
            header_lines += f"{key}: {value}\r\n"

        return f"{status_line}{header_lines}\r\n{self.body}".encode('utf-8')

    def _get_status_message(self):
        messages = {
            200: "OK",
            404: "Not Found",
            405: "Method Not Allowed"
        }
        return messages.get(self.status_code, "UNKNOWN")


if __name__ == "__main__":
    server = HTTPServer()
    server.start()
