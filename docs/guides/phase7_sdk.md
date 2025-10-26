# Phase 7 SDK Enhancements

Hyperion now provides lightweight SDKs for Python and TypeScript to access Phase 6/7 operational APIs (monitoring, deployment, autoscaling).

## Python SDK

Location: `sdk/python/hyperion_sdk`

### Installation

```bash
pip install -e sdk/python
```

### Usage

```python
from hyperion_sdk import HyperionClient

client = HyperionClient(base_url="https://hyperion.example.com", api_key="secret")

snapshot = client.get_monitoring_snapshot()
health = client.get_health()
autoscale = client.autoscale_plan(replicas=6)
```

## TypeScript SDK

Location: `sdk/typescript`

### Setup

```bash
cd sdk/typescript
npm install
npm run build
```

### Usage

```ts
import { HyperionClient } from "@hyperion/sdk";

const client = new HyperionClient({ baseUrl: "https://hyperion.example.com", apiKey: "secret" });

const monitoring = await client.getMonitoringSnapshot();
const autoscale = await client.autoscalePlan(6);
```

## Endpoints Covered

- `/api/monitoring`
- `/api/health`
- `/api/deploy/plan`
- `/api/deploy/apply`
- `/api/deploy/status`
- `/api/autoscale/plan`

Refer to `docs/examples/deployment_autoscaling.md` for end-to-end usage scenarios.
