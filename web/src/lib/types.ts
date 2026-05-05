export type GraphNode = {
  id: string;
  label: string;
  group: string;
  x: number;
  y: number;
  highlighted: boolean;
  active: boolean;
  dimmed: boolean;
  note: string;
};

export type GraphEdge = {
  id: string;
  source: string;
  target: string;
  label: string;
  highlighted: boolean;
  dashed: boolean;
};

export type GraphSnapshot = {
  layout: string;
  nodes: GraphNode[];
  edges: GraphEdge[];
};

export type TableRow = {
  cells: string[];
  highlighted: boolean;
};

export type TableSnapshot = {
  columns: string[];
  rows: TableRow[];
};

export type CompressionTrace = {
  frequencyTable: TableSnapshot;
  heapTable: TableSnapshot;
  treeSnapshot: GraphSnapshot;
  codeTable: TableSnapshot;
  bitString: string;
  compressedBytesHex: string;
  bitCount: number;
};

export type RangeMergeItem = {
  key: number;
  value: string;
  source: string;
};

export type RangeMergeTrace = {
  items: RangeMergeItem[];
};

export type TracePanel = {
  id: string;
  title: string;
  kind: "graph" | "table" | "compression" | "range-merge";
  description: string;
  bullets: string[];
  graph?: GraphSnapshot;
  table?: TableSnapshot;
  compression?: CompressionTrace;
  rangeMerge?: RangeMergeTrace;
};

export type ResultRow = {
  key: number;
  value: string;
  source: string;
};

export type OperationTrace = {
  command: string;
  commandType: string;
  success: boolean;
  statusMessage: string;
  resultSource: string;
  rows: ResultRow[];
  panels: TracePanel[];
};
