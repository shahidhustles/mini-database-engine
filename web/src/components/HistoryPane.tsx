import { CheckCircle2, CircleAlert, DatabaseZap } from "lucide-react";

import type { OperationTrace } from "../lib/types";

type HistoryPaneProps = {
  items: OperationTrace[];
  selectedIndex: number;
  onSelect: (index: number) => void;
};

export function HistoryPane({ items, selectedIndex, onSelect }: HistoryPaneProps) {
  return (
    <section className="history-pane">
      <div className="section-bar">
        <div className="section-label">
          <DatabaseZap size={15} />
          <span>History</span>
        </div>
      </div>

      <div className="history-list">
        {items.length === 0 ? (
          <div className="empty-state">Run a command to populate the workbench.</div>
        ) : (
          items.map((item, index) => (
            <button
              key={`${item.command}-${index}`}
              className={`history-item${selectedIndex === index ? " selected" : ""}`}
              onClick={() => onSelect(index)}
              type="button"
            >
              <div className="history-item-top">
                <span className="history-command">{item.command}</span>
                <span className={`status-chip ${item.success ? "ok" : "error"}`}>
                  {item.success ? <CheckCircle2 size={12} /> : <CircleAlert size={12} />}
                  <span>{item.commandType}</span>
                </span>
              </div>
              <div className="history-item-bottom">
                <span>{item.statusMessage}</span>
                <span>{item.resultSource || "n/a"}</span>
              </div>
            </button>
          ))
        )}
      </div>
    </section>
  );
}
