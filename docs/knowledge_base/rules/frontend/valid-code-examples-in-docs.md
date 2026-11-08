---
name: brave/valid-code-examples-in-docs
description:
  Code examples in README files and other documentation must be syntactically
  valid and compilable, because developers copy-paste them.
rubric: readability
severity: should_fix
---

**Rule.** Every code example a change adds to a README or other documentation
must parse and compile as written. In JSX, wrap sibling elements in a fragment
(`<>...</>`) and pass handlers as functions (`onClick={() => fn()}`), not as
calls (`onClick={fn()}`). An example presented as the wrong way to do something
shows its mistake on purpose and is not held to this.

**Why.** Developers copy-paste examples. An invalid one causes confusion and
compile errors, and an `onClick={fn()}` that does compile runs `fn` on every
render instead of on click.

## Examples

Bad:

```tsx
<div>Hello</div>
<button onClick={api.reset()}>Reset</button>
```

Good:

```tsx
<>
  <div>Hello</div>
  <button onClick={() => api.reset()}>Reset</button>
</>
```
