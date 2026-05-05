import type { TableSnapshot } from "../lib/types";

type TablePanelProps = {
  table: TableSnapshot;
};

export function TablePanel({ table }: TablePanelProps) {
  return (
    <div className="table-shell">
      <table className="trace-table">
        <thead>
          <tr>
            {table.columns.map((column) => (
              <th key={column}>{column}</th>
            ))}
          </tr>
        </thead>
        <tbody>
          {table.rows.map((row, index) => (
            <tr key={`${row.cells.join("-")}-${index}`} className={row.highlighted ? "highlighted" : ""}>
              {row.cells.map((cell, cellIndex) => (
                <td key={`${cell}-${cellIndex}`}>{cell}</td>
              ))}
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}
