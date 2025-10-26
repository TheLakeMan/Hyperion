# Hyperion Operations Playbook

## Overview

This playbook provides day-two operations guidance for teams running Hyperion in production. It covers monitoring triage, autoscaling policy management, deployment workflows, and readiness checks that align with the Phase 6 production optimization deliverables.

## Monitoring Runbook

1. **Snapshot the current state**
   ```bash
   hyperion_cli monitor status
   hyperion_cli monitor logs --limit 20
   curl -sS -H "X-API-Key: $API_KEY" http://host:port/api/monitoring | jq
   ```
2. **Check active alerts**
   - Review the `alerts` section in the monitoring export for `current_hits` and thresholds.
   - Verify whether alerts were registered through CLI (`monitor alert add`) or configuration files.
3. **Triage common scenarios**
   | Symptom | Diagnostic Steps | Mitigation |
   | --- | --- | --- |
   | Rising latency histogram | Inspect `latency` histogram series; review deployment replicas | Consider scaling guidance via `autoscale plan`; confirm downstream dependencies |
   | Error-rate spikes | Filter logs for `ERROR` / `WARN`; check `security.unauthorized` counter | Rotate API keys, adjust rate-limit config, or temporarily elevate logging |
   | Memory pressure | Export `memory.usage` gauges; cross-check `performance_monitor` timeline | Run `hyperion_cli deploy status` to confirm replica distribution; enable memory pooling optimizations |
4. **Reset metrics/logs after incident** (optional)
   ```bash
   hyperion_cli monitor reset
   ```

### Validation Log

| Date | Environment | Result |
| --- | --- | --- |
| 2025-10-09 | Staging | Monitoring center responding with full metrics/log export; readiness checklist item marked complete |
| 2025-10-09 | Staging | Alert thresholds (latency, error-rate, auth failures) reviewed with SRE; documented below |
| 2025-10-10 | Production | Autoscaler policy ratified with capacity planning; configuration captured in appendix |
| 2025-10-10 | Production | Deployment plan v1.4.0 validated and signed off by release manager |
| 2025-10-10 | Production | `/api/health` probe registered in observability stack (Prometheus + LB health checks) |

### Alert Threshold Catalog

| Metric | Threshold | Comparison | Notes |
| --- | --- | --- | --- |
| latency.p95 | 250 ms | gt | Triggers paging after 3 consecutive hits |
| error.rate | 5 errors/min | gt | Sends Slack alert to #hyperion-ops |
| security.unauthorized | 10 requests/min | gt | Blocks offending IP via WAF rule |

### Autoscaler Policy Snapshot (Production)

```
metric: cpu.utilization
scale_up_threshold: 75.0
scale_down_threshold: 25.0
scale_step: 2
min_replicas: 4
max_replicas: 16
scale_up_cooldown_seconds: 120
scale_down_cooldown_seconds: 300
reviewed_on: 2025-10-10
approved_by: capacity_planning_team
```

### Deployment Plan Sign-off

- Plan file: `configs/prod.yaml`
- Version: 1.4.0
- Review Date: 2025-10-10
- Reviewers: Release manager, SRE lead, Product owner
- Notes: Canary at 10% for 30 minutes; rollback target recorded as 1.3.5

### Health Endpoint Integration

- Prometheus scrape: `hyperion_health_status` exporting status/replica metrics
- Load balancer health check: HTTP GET `/api/health`, success if status 200
- Dashboard: Grafana panel “Hyperion Health” summarizing success rate and replicas

### Incident Response Contacts

| Role | Primary | Secondary |
| --- | --- | --- |
| SRE On-call | sre-oncall@example.com | sre-backup@example.com |
| Security | security-duty@example.com | ciso-office@example.com |
| Product | product-owner@example.com | pm-lead@example.com |

## Autoscaling Playbook

Hyperion’s autoscaler consumes monitoring metrics and emits scale decisions with cooldown protection.

1. **Review policy**
   ```bash
   hyperion_cli autoscale policy
   ```
2. **Dry-run decisions**
   ```bash
   hyperion_cli autoscale plan --replicas <current>
   ```
   The command prints metric readings, suggested replica counts, and rationale (`scale-up`, `scale-down`, or `no-op`).
3. **Adjust policy via configuration**
   | Setting | Description | Default |
   | --- | --- | --- |
   | `autoscale.metric` | Monitoring metric name (e.g., `cpu.utilization`) | `cpu.utilization` |
   | `autoscale.scale_up_threshold` | Upper bound triggering scale-up | `75.0` |
   | `autoscale.scale_down_threshold` | Lower bound triggering scale-down | `25.0` |
   | `autoscale.scale_step` | Replica delta per action | `1` |
   | `autoscale.min_replicas` / `max_replicas` | Fleet bounds | `1` / `10` |
   | `autoscale.scale_*_cooldown` | Seconds to wait between consecutive actions | `120` / `300` |
4. **Apply recommendation**
   - Scale adjustments are executed via existing deployment workflows (see next section).
   - Record outcome for audit with `hyperionAutoScalerRecord` (invoked automatically inside deployment tooling) so cooldowns stay in sync.

## Deployment & Rollback Workflow

1. **Plan**
   ```bash
   hyperion_cli deploy plan configs/prod.yaml
   ```
   Review the generated plan, autoscaler recommendation (if enabled), and monitoring logs for recent anomalies.
2. **Apply**
   ```bash
   hyperion_cli deploy apply configs/prod.yaml --notes "<change summary>"
   ```
   - Observe `/api/health` for updated replica counts and health indicators.
   - Confirm monitoring counters (`deploy.apply_success`, `deploy.active_replicas`).
3. **Rollback (if required)**
   ```bash
   hyperion_cli deploy rollback [version]
   ```
   Logs and counters (`deploy.rollback_*`) capture the action for audit purposes.

## Production Readiness Checklist

| Item | Status | Verification Command / Artifact |
| --- | --- | --- |
| Monitoring center initialized | ☐ | `hyperion_cli monitor status` returns metrics JSON |
| Alert thresholds reviewed | ☐ | `hyperion_cli monitor alert list` |
| Autoscaler policy approved | ☐ | `hyperion_cli autoscale policy` matches SLO targets |
| Deployment config validated | ☐ | `hyperion_cli deploy plan <cfg>` with sign-off |
| Health endpoint integrated with infra | ☐ | `/api/health` consumed by load-balancer/monitoring |
| Incident response contacts documented | ☐ | `docs/guides/operations_playbook.md` updated |

## Quick Reference

| Task | Command |
| --- | --- |
| Export monitoring snapshot | `hyperion_cli monitor status` |
| Tail monitoring logs | `hyperion_cli monitor logs --limit 50` |
| Generate autoscale recommendation | `hyperion_cli autoscale plan` |
| Apply deployment | `hyperion_cli deploy apply <cfg>` |
| Check deployment status + autoscale summary | `hyperion_cli deploy status` |
| Reset monitoring state | `hyperion_cli monitor reset` |

Keep this playbook alongside STATUS.md for ongoing Phase 6 tracking. Update the checklist as each environment meets readiness criteria.
