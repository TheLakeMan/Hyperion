# Phase 7 Roadmap – Post-Launch Enablement

## Support Strategy

- **Channels**
  - *LTS*: 12-month support window with quarterly patch releases.
  - *Rapid*: 3-month cadence for innovation updates; requires latest SDKs.
- **SLOs**
  - Critical fixes < 48h for production blockers.
  - Security advisories within 24h of disclosure.

## Near-term Initiatives (Q1–Q2)

1. **Observability Dashboards**
   - Grafana bundles for operations, security, autoscaling.
   - Prometheus exporter for monitoring JSON ingest.

2. **Ecosystem Integrations**
   - Terraform module for Hyperion deployment manager + autoscaler.
   - Helm chart for Kubernetes operator.

3. **Developer Tooling**
   - SDK automation in CI templates.
   - Code samples for Python/TypeScript clients.

## Mid-term Initiatives (Q3–Q4)

- Forecast-driven autoscaling (machine learning-based).
- Anomaly detection service leveraging monitoring timelines.
- Managed connectors for AWS/GCP observability stacks.

## Governance & Reviews

- Monthly ops review: metrics, alerts, incidents.
- Quarterly security audit + dependency review.
- Biannual roadmap refresh aligned with community feedback.

## Milestone Tracking

| Milestone | Target Date | Status |
| --- | --- | --- |
| SDK & dashboard public beta | 2026-01-15 | Planned |
| Terraform/Helm packages GA | 2026-03-01 | Planned |
| Forecast autoscaler prototype | 2026-06-15 | Planned |

Keep this document updated alongside STATUS.md to reflect ongoing progress.
