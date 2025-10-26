export interface HyperionClientOptions {
  baseUrl: string;
  apiKey?: string;
  timeoutMs?: number;
}

export class HyperionClient {
  private readonly baseUrl: string;
  private readonly apiKey?: string;
  private readonly timeoutMs: number;

  constructor(options: HyperionClientOptions) {
    this.baseUrl = options.baseUrl.replace(/\/$/, "");
    this.apiKey = options.apiKey;
    this.timeoutMs = options.timeoutMs ?? 10000;
  }

  private async request<T>(path: string, init?: RequestInit): Promise<T> {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), this.timeoutMs);

    const headers: Record<string, string> = {
      "Content-Type": "application/json",
      ...(init?.headers as Record<string, string> | undefined)
    };

    if (this.apiKey) {
      headers["X-API-Key"] = this.apiKey;
    }

    try {
      const response = await fetch(`${this.baseUrl}${path}`, {
        ...init,
        headers,
        signal: controller.signal
      });

      clearTimeout(timeout);

      if (!response.ok) {
        const detail = await response.text();
        throw new HyperionClientError(`HTTP ${response.status} for ${path}: ${detail}`);
      }

      if (response.status === 204) {
        return {} as T;
      }

      return (await response.json()) as T;
    } catch (error) {
      if (error instanceof HyperionClientError) {
        throw error;
      }
      throw new HyperionClientError(`Failed to reach Hyperion: ${(error as Error).message}`);
    }
  }

  async getMonitoringSnapshot(): Promise<unknown> {
    return this.request("/api/monitoring");
  }

  async getHealth(): Promise<unknown> {
    return this.request("/api/health");
  }

  async autoscalePlan(replicas?: number): Promise<unknown> {
    const body = replicas !== undefined ? JSON.stringify({ replicas }) : undefined;
    return this.request("/api/autoscale/plan", {
      method: "POST",
      body
    });
  }

  async deploymentPlan(config: unknown): Promise<unknown> {
    return this.request("/api/deploy/plan", {
      method: "POST",
      body: JSON.stringify(config)
    });
  }

  async deploymentApply(config: unknown): Promise<unknown> {
    return this.request("/api/deploy/apply", {
      method: "POST",
      body: JSON.stringify(config)
    });
  }

  async deploymentStatus(): Promise<unknown> {
    return this.request("/api/deploy/status");
  }
}

export class HyperionClientError extends Error {}
