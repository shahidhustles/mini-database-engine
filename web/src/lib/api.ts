import type { OperationTrace } from "./types";

export async function fetchHealth(): Promise<boolean> {
  const response = await fetch("/api/health");
  if (!response.ok) {
    return false;
  }
  const body = (await response.json()) as { ok?: boolean };
  return body.ok === true;
}

export async function executeCommand(command: string): Promise<OperationTrace> {
  const response = await fetch("/api/execute", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify({ command }),
  });

  const body = (await response.json()) as OperationTrace;
  if (!response.ok) {
    throw new Error(body.statusMessage || "Request failed.");
  }
  return body;
}
