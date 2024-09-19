from http.server import BaseHTTPRequestHandler, HTTPServer

class SimpleHTTPRequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        # 设置响应状态码和响应头
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        
        # 编写响应内容
        response_content = """
        <!DOCTYPE html>
        <html>
        <head>
            <title>Simple HTTP Server</title>
        </head>
        <body>
            <h1>Welcome to the Simple HTTP Server!</h1>
            <p>This is a response from a Python HTTP server.</p>
        </body>
        </html>
        """
        
        # 发送响应内容
        self.wfile.write(response_content.encode('utf-8'))

def run(server_class=HTTPServer, handler_class=SimpleHTTPRequestHandler, port=8080):
    server_address = ('192.168.182.3', port)
    httpd = server_class(server_address, handler_class)
    print(f'Starting httpd server on port {port}...')
    httpd.serve_forever()

if __name__ == '__main__':
    run()
