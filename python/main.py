"""Serve the keyboard controls on the VENTUNO Q's Linux processor."""

from http.server import BaseHTTPRequestHandler, HTTPServer
import json
from pathlib import Path
import secrets

from arduino.app_utils import Bridge


PORT = 8765
PAGE = Path(__file__).with_name("keyboard.html").read_bytes()
TOKEN = secrets.token_urlsafe(12)


class DriveHandler(BaseHTTPRequestHandler):
    timeout = 0.5

    def reply(self, status, data, kind="application/json; charset=utf-8"):
        body = data if isinstance(data, bytes) else json.dumps(data).encode()
        self.send_response(status)
        self.send_header("Content-Type", kind)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("X-Frame-Options", "DENY")
        self.end_headers()
        try:
            self.wfile.write(body)
        except (BrokenPipeError, ConnectionResetError):
            pass

    def do_GET(self):
        if self.path == "/":
            self.reply(200, PAGE, "text/html; charset=utf-8")
        else:
            self.reply(404, {"error": "Not found"})

    def do_POST(self):
        if self.path != "/drive":
            self.reply(404, {"error": "Not found"})
            return
        supplied = self.headers.get("Authorization", "").encode()
        if not secrets.compare_digest(supplied, f"Bearer {TOKEN}".encode()):
            self.reply(401, {"error": "App Lab 콘솔의 조종 코드 확인"})
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if not 0 < length <= 64 or self.headers.get_content_type() != "application/json":
                raise ValueError
            command = json.loads(self.rfile.read(length))
            if not isinstance(command, list) or len(command) != 2:
                raise ValueError
            if any(type(value) is not int or value not in (-1, 0, 1) for value in command):
                raise ValueError
        except (ValueError, UnicodeError, TimeoutError):
            self.reply(400, {"error": "Invalid command"})
            return
        try:
            result = Bridge.call("rc_set_drive", *command, timeout=0.18)
            left, right = map(int, result.split(","))
        except Exception:
            self.reply(503, {"error": "STM32 응답 없음 또는 조향 초기화 실패"})
            return
        self.reply(200, {"left": left, "right": right})

    def log_request(self, code="-", size="-"):
        # Avoid flooding the App Lab console with recurring drive requests.
        if self.path != "/drive":
            super().log_request(code, size)


if __name__ == "__main__":
    with HTTPServer(("0.0.0.0", PORT), DriveHandler) as server:
        print(f"Keyboard page port: {PORT}\n조종 코드: {TOKEN}", flush=True)
        server.serve_forever()
