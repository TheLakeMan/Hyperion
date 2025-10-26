import http from "node:http";
import { HyperionClient } from "../dist/index.js";

function createServer() {
  return http.createServer((req, res) => {
    const { method, url } = req;
    res.setHeader("Content-Type", "application/json");

    if (method === "GET" && url === "/api/monitoring") {
      res.end(JSON.stringify({ metrics: [] }));
    } else if (method === "GET" && url === "/api/health") {
      res.end(JSON.stringify({ active_replicas: 4 }));
    } else if (method === "GET" && url === "/api/deploy/status") {
      res.end(JSON.stringify({ last_state: "SUCCEEDED" }));
    } else if (method === "POST" && url === "/api/autoscale/plan") {
      let body = "";
      req.on("data", chunk => (body += chunk));
      req.on("end", () => {
        const payload = body ? JSON.parse(body) : {};
        const replicas = payload.replicas ?? 4;
        res.end(JSON.stringify({ decision: replicas < 6 ? "scale-up" : "no-op" }));
      });
      return;
    } else if (method === "POST" && url === "/api/deploy/plan") {
      let body = "";
      req.on("data", chunk => (body += chunk));
      req.on("end", () => {
        const payload = body ? JSON.parse(body) : {};
        res.end(JSON.stringify({ plan: ["scale", payload.desired_replicas ?? 4] }));
      });
      return;
    } else if (method === "POST" && url === "/api/deploy/apply") {
      res.end(JSON.stringify({ status: "applied" }));
    } else {
      res.statusCode = 404;
      res.end(JSON.stringify({ error: "not found" }));
    }
  });
}

async function run() {
  const server = createServer();
  await new Promise(resolve => server.listen(0, resolve));
  const { port } = server.address();

  const client = new HyperionClient({ baseUrl: `http://127.0.0.1:${port}`, apiKey: "test" });

  const monitoring = await client.getMonitoringSnapshot();
  if (!("metrics" in monitoring)) {
    throw new Error("Monitoring snapshot missing metrics field");
  }

  const autoscale = await client.autoscalePlan(3);
  if (autoscale.decision !== "scale-up") {
    throw new Error(`Unexpected autoscale decision: ${autoscale.decision}`);
  }

  const plan = await client.deploymentPlan({ desired_replicas: 8 });
  if (plan.plan[1] !== 8) {
    throw new Error("Deployment plan returned unexpected value");
  }

  await client.deploymentApply({ desired_replicas: 8 });
  await client.deploymentStatus();

  server.close();
}

run().catch(error => {
  console.error(error);
  process.exit(1);
});
