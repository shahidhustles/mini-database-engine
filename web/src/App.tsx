import { useEffect, useMemo, useState } from "react";
import { Activity, Database, Layers3 } from "lucide-react";

import { CommandComposer } from "./components/CommandComposer";
import { HistoryPane } from "./components/HistoryPane";
import { TracePanelRenderer } from "./components/TracePanelRenderer";
import { executeCommand, fetchHealth } from "./lib/api";
import type { OperationTrace } from "./lib/types";

const DEFAULT_COMMAND = 'INSERT 5 "hello world"';

export function App() {
  const [command, setCommand] = useState(DEFAULT_COMMAND);
  const [history, setHistory] = useState<OperationTrace[]>([]);
  const [selectedIndex, setSelectedIndex] = useState(0);
  const [running, setRunning] = useState(false);
  const [apiHealthy, setApiHealthy] = useState<boolean | null>(null);

  useEffect(() => {
    fetchHealth()
      .then(setApiHealthy)
      .catch(() => setApiHealthy(false));
  }, []);

  const selected = history[selectedIndex];

  async function runCommand() {
    if (!command.trim()) {
      return;
    }
    setRunning(true);
    try {
      const trace = await executeCommand(command.trim());
      setHistory((current) => [trace, ...current]);
      setSelectedIndex(0);
    } catch (error) {
      const failure: OperationTrace = {
        command,
        commandType: "ERROR",
        success: false,
        statusMessage: error instanceof Error ? error.message : "Unknown error.",
        resultSource: "none",
        rows: [],
        panels: [],
      };
      setHistory((current) => [failure, ...current]);
      setSelectedIndex(0);
    } finally {
      setRunning(false);
    }
  }

  const summary = useMemo(() => {
    if (!selected) {
      return "No command selected.";
    }
    if (selected.rows.length === 0) {
      return selected.statusMessage;
    }
    return `${selected.rows.length} result row${selected.rows.length === 1 ? "" : "s"} from ${selected.resultSource || "n/a"}.`;
  }, [selected]);

  return (
    <div className="app-shell">
      <header className="topbar">
        <div className="brand">
          <Database size={18} />
          <div>
            <div className="brand-name">Mini DB Workbench</div>
            <div className="brand-subtle">C++ engine trace surface</div>
          </div>
        </div>
        <div className="topbar-meta">
          <div className={`health-indicator ${apiHealthy ? "ok" : apiHealthy === false ? "error" : ""}`}>
            <Activity size={14} />
            <span>{apiHealthy ? "API online" : apiHealthy === false ? "API offline" : "Checking API"}</span>
          </div>
        </div>
      </header>

      <main className="workspace">
        <aside className="left-rail">
          <CommandComposer
            command={command}
            onCommandChange={setCommand}
            onRun={runCommand}
            onReset={() => setCommand(DEFAULT_COMMAND)}
            running={running}
          />
          <HistoryPane items={history} selectedIndex={selectedIndex} onSelect={setSelectedIndex} />
        </aside>

        <section className="content">
          <section className="result-strip">
            <div className="section-bar">
              <div className="section-label">
                <Layers3 size={15} />
                <span>Operation</span>
              </div>
            </div>
            {selected ? (
              <div className="result-content">
                <div className="result-command">{selected.command}</div>
                <div className="result-meta">
                  <span className={`status-chip ${selected.success ? "ok" : "error"}`}>
                    {selected.commandType}
                  </span>
                  <span className="plain-chip">{selected.resultSource || "n/a"}</span>
                </div>
                <div className="result-summary">{summary}</div>
                {selected.rows.length > 0 ? (
                  <div className="table-shell compact">
                    <table className="trace-table">
                      <thead>
                        <tr>
                          <th>Key</th>
                          <th>Value</th>
                          <th>Source</th>
                        </tr>
                      </thead>
                      <tbody>
                        {selected.rows.map((row) => (
                          <tr key={`${row.key}-${row.source}`}>
                            <td>{row.key}</td>
                            <td>{row.value}</td>
                            <td>{row.source}</td>
                          </tr>
                        ))}
                      </tbody>
                    </table>
                  </div>
                ) : null}
              </div>
            ) : (
              <div className="empty-state">Select or run a command to inspect its trace.</div>
            )}
          </section>

          <section className="trace-grid">
            {selected?.panels.length ? (
              selected.panels.map((panel) => <TracePanelRenderer key={panel.id} panel={panel} />)
            ) : (
              <div className="empty-state">No trace panels yet.</div>
            )}
          </section>
        </section>
      </main>
    </div>
  );
}
