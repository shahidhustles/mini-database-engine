import CytoscapeComponent from "react-cytoscapejs";

import type { GraphSnapshot } from "../lib/types";

type GraphPanelProps = {
  snapshot: GraphSnapshot;
};

function buildElements(snapshot: GraphSnapshot) {
  const nodes = snapshot.nodes.map((node) => ({
    data: {
      id: node.id,
      label: node.label,
      note: node.note,
    },
    classes: [
      node.group,
      node.highlighted ? "highlighted" : "",
      node.active ? "active" : "",
      node.dimmed ? "dimmed" : "",
    ]
      .filter(Boolean)
      .join(" "),
    position: {
      x: node.x,
      y: node.y,
    },
  }));

  const edges = snapshot.edges.map((edge) => ({
    data: {
      id: edge.id,
      source: edge.source,
      target: edge.target,
      label: edge.label,
    },
    classes: [
      edge.highlighted ? "highlighted" : "",
      edge.dashed ? "dashed" : "",
    ]
      .filter(Boolean)
      .join(" "),
  }));

  return [...nodes, ...edges];
}

const stylesheet = [
  {
    selector: "node",
    style: {
      width: 76,
      height: 44,
      shape: "round-rectangle",
      "background-color": "#ffffff",
      "border-width": 1,
      "border-color": "#c6c8cf",
      label: "data(label)",
      color: "#17181c",
      "font-size": 10,
      "text-wrap": "wrap",
      "text-max-width": 66,
      "font-family": "IBM Plex Sans",
      "text-valign": "center",
      "text-halign": "center",
    },
  },
  {
    selector: "node.rb-red",
    style: {
      "background-color": "#f5dddd",
      "border-color": "#d48383",
    },
  },
  {
    selector: "node.rb-black",
    style: {
      "background-color": "#efeff2",
      "border-color": "#6b6d76",
    },
  },
  {
    selector: "node.skip-node",
    style: {
      width: 64,
      height: 34,
      "background-color": "#f6f4ee",
      "border-color": "#b7ab8b",
    },
  },
  {
    selector: "node.bplus-leaf",
    style: {
      width: 120,
      height: 50,
      "background-color": "#f0f3ed",
      "border-color": "#8fa083",
    },
  },
  {
    selector: "node.bplus-internal",
    style: {
      width: 120,
      height: 50,
      "background-color": "#f4f4f6",
      "border-color": "#8c8f97",
    },
  },
  {
    selector: "node.huffman-leaf",
    style: {
      width: 92,
      height: 44,
      "background-color": "#f8efe7",
      "border-color": "#d2a581",
    },
  },
  {
    selector: "node.huffman-merge",
    style: {
      width: 72,
      height: 40,
      "background-color": "#f1f2f4",
      "border-color": "#7f8592",
    },
  },
  {
    selector: "node.highlighted",
    style: {
      "border-width": 2,
      "border-color": "#bc5f2f",
    },
  },
  {
    selector: "node.active",
    style: {
      "background-color": "#fff4d8",
    },
  },
  {
    selector: "edge",
    style: {
      width: 1.5,
      "line-color": "#81848d",
      "target-arrow-color": "#81848d",
      "target-arrow-shape": "none",
      "curve-style": "bezier",
      label: "data(label)",
      "font-size": 9,
      color: "#5f6470",
      "font-family": "IBM Plex Mono",
      "text-background-opacity": 1,
      "text-background-color": "#fbfbfc",
      "text-background-padding": 2,
    },
  },
  {
    selector: "edge.highlighted",
    style: {
      width: 2.5,
      "line-color": "#bc5f2f",
      "target-arrow-color": "#bc5f2f",
    },
  },
  {
    selector: "edge.dashed",
    style: {
      "line-style": "dashed",
    },
  },
];

export function GraphPanel({ snapshot }: GraphPanelProps) {
  const elements = buildElements(snapshot);
  const layout =
    snapshot.layout === "preset"
      ? { name: "preset", fit: true, padding: 20 }
      : { name: "breadthfirst", directed: true, fit: true, padding: 20, spacingFactor: 1.1 };

  return (
    <div className="graph-shell">
      <CytoscapeComponent
        elements={elements}
        stylesheet={stylesheet}
        layout={layout}
        autoungrabify
        userZoomingEnabled={false}
        userPanningEnabled={false}
        boxSelectionEnabled={false}
        style={{ width: "100%", height: "100%" }}
      />
    </div>
  );
}
