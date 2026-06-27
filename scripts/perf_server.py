from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
import time

class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path.startswith("/delay"):
            time.sleep(0.2)

        body = b"OK"

        self.send_response(200)
        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, format, *args):
        return

if __name__ == "__main__":
    server = ThreadingHTTPServer(("127.0.0.1", 9090), Handler)
    print("perf server listening on http://127.0.0.1:9090")
    server.serve_forever()