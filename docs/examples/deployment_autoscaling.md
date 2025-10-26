# Deployment & Autoscaling Tutorial

This walkthrough demonstrates how to combine Hyperion’s deployment manager, monitoring center, and autoscaler to deliver a resilient production rollout.

## Prerequisites

- Hyperion built with Phase 6 features (monitoring, deployment manager, autoscaler).
- API key configured in `hyperion.conf` if the web server endpoints are protected.
- A deployment configuration file (e.g., `configs/prod.yaml`).

## 1. Bootstrap Monitoring

```bash
# Ensure monitoring center is initialized
hyperion_cli monitor status

# Optionally tail recent logs
hyperion_cli monitor logs --limit 10
```

Verify the `/api/monitoring` endpoint:

```bash
curl -sS -H "X-API-Key: $API_KEY" http://localhost:8080/api/monitoring | jq
```

## 2. Review Autoscale Policy

```bash
hyperion_cli autoscale policy
```

Adjust thresholds via config overrides if needed:

```bash
hyperion_cli config autoscale.scale_up_threshold 80
hyperion_cli config autoscale.scale_down_threshold 20
```

## 3. Generate Deployment Plan

```bash
hyperion_cli deploy plan configs/prod.yaml
```

Inspect the output for:

- Planned replica changes.
- Autoscaler recommendation (if enabled) displayed in the CLI.
- Monitoring logs indicating prior deployment events.

## 4. Execute Deployment

```bash
hyperion_cli deploy apply configs/prod.yaml --notes "blue/green rollout"
```

Track metrics and logs during rollout:

```bash
watch -n 5 hyperion_cli monitor status
```

## 5. Validate Health Endpoint

```bash
curl -sS -H "X-API-Key: $API_KEY" http://localhost:8080/api/health | jq
```

Ensure `active_replicas`, `success_rate`, and health summary align with expectations.

## 6. Evaluate Autoscale Recommendation

```bash
hyperion_cli autoscale plan
```

- `Decision: scale-up` or `scale-down` indicates action to consider.
- Apply adjustments via `deploy apply` for declarative control.

## 7. Rollback (if required)

```bash
hyperion_cli deploy rollback <version>
```

Monitor `deploy.rollback_*` counters:

```bash
hyperion_cli monitor status | jq '.metrics[] | select(.name | contains("deploy.rollback"))'
```

## 8. Post-Deployment Checklist

Update `STATUS.md` readiness table and `operations_playbook.md` with:

- Deployment summary & notes.
- Autoscale decision rationale.
- Any follow-up actions for alert thresholds or incident contacts.

## Appendix: Sample Configuration

```yaml
# configs/prod.yaml
environment: production
version: 1.4.0
artifact_path: artifacts/hyperion-1.4.0.tar.gz
cluster: prod-cluster
desired_replicas: 6
enable_canary: true
canary_traffic_percent: 10
health_initial_delay_seconds: 30
health_interval_seconds: 15
max_parallel: 2
```

Use this tutorial as a baseline and customize commands to match your infrastructure orchestration pipeline (CI/CD, IaC, etc.).
