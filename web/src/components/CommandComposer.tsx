import { Command, Play, RotateCcw } from "lucide-react";

type CommandComposerProps = {
  command: string;
  onCommandChange: (value: string) => void;
  onRun: () => void;
  onReset: () => void;
  running: boolean;
};

const EXAMPLES = [
  'INSERT 8 "kiwi"',
  "GET 8",
  "RANGE 1 10",
  "FLUSH",
];

export function CommandComposer({
  command,
  onCommandChange,
  onRun,
  onReset,
  running,
}: CommandComposerProps) {
  return (
    <section className="composer">
      <div className="section-bar">
        <div className="section-label">
          <Command size={15} />
          <span>Command</span>
        </div>
      </div>

      <label className="field-label" htmlFor="command-input">
        Raw database command
      </label>
      <div className="command-row">
        <input
          id="command-input"
          className="command-input"
          value={command}
          onChange={(event) => onCommandChange(event.target.value)}
          onKeyDown={(event) => {
            if (event.key === "Enter" && !event.shiftKey) {
              event.preventDefault();
              onRun();
            }
          }}
          placeholder='INSERT 5 "hello world"'
          autoComplete="off"
          spellCheck={false}
        />
        <button className="primary-button" onClick={onRun} disabled={running || !command.trim()}>
          <Play size={14} />
          <span>{running ? "Running" : "Run"}</span>
        </button>
        <button className="secondary-button" onClick={onReset}>
          <RotateCcw size={14} />
        </button>
      </div>

      <div className="example-strip">
        {EXAMPLES.map((example) => (
          <button
            key={example}
            className="example-button"
            onClick={() => onCommandChange(example)}
            type="button"
          >
            {example}
          </button>
        ))}
      </div>
    </section>
  );
}
