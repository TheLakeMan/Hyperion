from __future__ import annotations

import json
from dataclasses import dataclass
from typing import Any, Dict, Optional

import urllib.request
import urllib.error


@dataclass
class HyperionClient:
    """Simple HTTP client for Hyperion operational APIs."""

    base_url: str
    api_key: Optional[str] = None
    timeout: int = 10

    def _request(self, path: str, method: str = "GET", data: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
        url = self.base_url.rstrip("/") + path
        headers = {"Content-Type": "application/json"}
        if self.api_key:
            headers["X-API-Key"] = self.api_key

        req_data = None
        if data is not None:
            req_data = json.dumps(data).encode("utf-8")

        request = urllib.request.Request(url, data=req_data, headers=headers, method=method)

        try:
            with urllib.request.urlopen(request, timeout=self.timeout) as response:
                charset = response.headers.get_content_charset() or "utf-8"
                payload = response.read().decode(charset)
                if not payload:
                    return {}
                return json.loads(payload)
        except urllib.error.HTTPError as exc:  # type: ignore[attr-defined]
            detail = exc.read().decode("utf-8") if exc.fp else exc.reason
            raise HyperionClientError(f"HTTP {exc.code} for {method} {path}: {detail}") from exc
        except urllib.error.URLError as exc:
            raise HyperionClientError(f"Unable to reach Hyperion at {url}: {exc.reason}") from exc

    # Monitoring endpoints -------------------------------------------------

    def get_monitoring_snapshot(self) -> Dict[str, Any]:
        """Return metrics/log snapshot from /api/monitoring."""

        return self._request("/api/monitoring")

    def get_health(self) -> Dict[str, Any]:
        """Return deployment health from /api/health."""

        return self._request("/api/health")

    # Autoscaling ----------------------------------------------------------

    def autoscale_plan(self, replicas: Optional[int] = None) -> Dict[str, Any]:
        payload: Optional[Dict[str, Any]] = None
        if replicas is not None:
            payload = {"replicas": replicas}
        return self._request("/api/autoscale/plan", method="POST", data=payload)

    # Deployment management -------------------------------------------------

    def deployment_plan(self, config: Dict[str, Any]) -> Dict[str, Any]:
        return self._request("/api/deploy/plan", method="POST", data=config)

    def deployment_apply(self, config: Dict[str, Any]) -> Dict[str, Any]:
        return self._request("/api/deploy/apply", method="POST", data=config)

    def deployment_status(self) -> Dict[str, Any]:
        return self._request("/api/deploy/status")


class HyperionClientError(RuntimeError):
    """Raised for Hyperion SDK communication errors."""


__all__ = ["HyperionClient", "HyperionClientError"]
