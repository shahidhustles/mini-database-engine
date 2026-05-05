import { Binary, GitBranch, Rows3, ScanSearch } from "lucide-react";

import type { CompressionTrace, RangeMergeTrace, TracePanel } from "../lib/types";
import { GraphPanel } from "./GraphPanel";
import { TablePanel } from "./TablePanel";

function CompressionPanel({ trace }: { trace: CompressionTrace }) {
  return (
    <div className="compression-grid">
      <div className="subpanel">
        <div className="subpanel-label">Frequency table</div>
        <TablePanel table={trace.frequencyTable} />
      </div>
      <div className="subpanel">
        <div className="subpanel-label">Min-heap merges</div>
        <TablePanel table={trace.heapTable} />
      </div>
      <div className="subpanel wide">
        <div className="subpanel-label">Huffman tree</div>
        <GraphPanel snapshot={trace.treeSnapshot} />
      </div>
      <div className="subpanel">
        <div className="subpanel-label">Code table</div>
        <TablePanel table={trace.codeTable} />
      </div>
      <div className="subpanel">
        <div className="subpanel-label">Encoded payload</div>
        <div className="binary-block">
          <div>{trace.bitString || "0"}</div>
          <div className="binary-meta">
            <span>{trace.bitCount} bits</span>
            <span>{trace.compressedBytesHex || "00"}</span>
          </div>
        </div>
      </div>
    </div>
  );
}

function RangeMergePanel({ trace }: { trace: RangeMergeTrace }) {
  return (
    <div className="table-shell">
      <table className="trace-table">
        <thead>
          <tr>
            <th>Key</th>
            <th>Value</th>
            <th>Source</th>
          </tr>
        </thead>
        <tbody>
          {trace.items.map((item) => (
            <tr key={`${item.key}-${item.source}`}>
              <td>{item.key}</td>
              <td>{item.value}</td>
              <td>{item.source}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}

const icons = {
  graph: ScanSearch,
  table: Rows3,
  compression: Binary,
  "range-merge": GitBranch,
} as const;

export function TracePanelRenderer({ panel }: { panel: TracePanel }) {
  const Icon = icons[panel.kind];

  return (
    <section className="trace-panel">
      <div className="trace-panel-head">
        <div className="section-label">
          <Icon size={15} />
          <span>{panel.title}</span>
        </div>
        <div className="trace-kind">{panel.kind}</div>
      </div>

      <p className="trace-description">{panel.description}</p>

      {panel.bullets.length > 0 ? (
        <ul className="bullet-list">
          {panel.bullets.map((bullet) => (
            <li key={bullet}>{bullet}</li>
          ))}
        </ul>
      ) : null}

      {panel.graph ? <GraphPanel snapshot={panel.graph} /> : null}
      {panel.table ? <TablePanel table={panel.table} /> : null}
      {panel.compression ? <CompressionPanel trace={panel.compression} /> : null}
      {panel.rangeMerge ? <RangeMergePanel trace={panel.rangeMerge} /> : null}
    </section>
  );
}
