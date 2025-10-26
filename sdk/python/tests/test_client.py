import json
import threading
import time
import unittest
from http.server import BaseHTTPRequestHandler, HTTPServer

from hyperion_sdk import HyperionClient


class _MockHandler(BaseHTTPRequestHandler):
    RESPONSES = {
        ("GET", "/api/monitoring"): ({"metrics": []}, 200),
        ("GET", "/api/health"): ({"active_replicas": 4}, 200),
        ("GET", "/api/deploy/status"): ({"last_state": "SUCCEEDED"}, 200),
    }

    def _write_json(self, payload, status=200):
        body = json.dumps(payload).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):  # noqa: N802 (BaseHTTPRequestHandler naming)
        payload, status = self.RESPONSES.get(("GET", self.path), ({"error": "not found"}, 404))
        self._write_json(payload, status)

    def do_POST(self):  # noqa: N802
        length = int(self.headers.get("Content-Length", "0"))
        data = self.rfile.read(length).decode("utf-8") if length else "{}"
        payload = json.loads(data)
        if self.path == "/api/autoscale/plan":
            replicas = payload.get("replicas", 4)
            self._write_json({"decision": "scale-up" if replicas < 6 else "no-op"})
        elif self.path == "/api/deploy/plan":
            self._write_json({"plan": ["scale", payload.get("desired_replicas", 4)]})
        elif self.path == "/api/deploy/apply":
            self._write_json({"status": "applied"})
        else:
            self._write_json({"error": "unsupported"}, 404)

    def log_message(self, format, *args):  # noqa: A003 - silence test server logs
        return


class HyperionClientTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.server = HTTPServer(("127.0.0.1", 0), _MockHandler)
        cls.port = cls.server.server_port
        cls.thread = threading.Thread(target=cls.server.serve_forever, daemon=True)
        cls.thread.start()
        time.sleep(0.1)

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown()
        cls.thread.join(timeout=2)

    def setUp(self):
        self.client = HyperionClient(base_url=f"http://127.0.0.1:{self.port}", api_key="test")

    def test_monitoring_snapshot(self):
        snapshot = self.client.get_monitoring_snapshot()
        self.assertIn("metrics", snapshot)

    def test_autoscale_plan(self):
        decision = self.client.autoscale_plan(replicas=3)
        self.assertEqual(decision["decision"], "scale-up")

    def test_deployment_plan_and_apply(self):
        plan = self.client.deployment_plan({"desired_replicas": 6})
        self.assertEqual(plan["plan"], ["scale", 6])
        result = self.client.deployment_apply({"desired_replicas": 6})
        self.assertEqual(result["status"], "applied")


if __name__ == "__main__":  # pragma: no cover - direct execution support
    unittest.main()
