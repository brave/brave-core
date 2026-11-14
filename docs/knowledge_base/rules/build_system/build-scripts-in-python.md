---
name: brave/build-scripts-in-python
description:
  'New build and utility scripts in brave-core are written in Python run with
  vpython, not Node.js or shell.'
rubric: design
severity: should_fix
---

**Rule.** Write a new build or utility script (a GN `action()` script, a `DEPS`
hook, a developer tool) in Python, run with `vpython3` from depot_tools. Do not
add new Node.js (`.js`, `.mjs`) or shell (`.sh`, `.bash`) scripts for this.

**Why.** Python follows Chromium's conventions, needs no runtime beyond the one
depot_tools already provides, and works on every platform including Windows,
where shell scripts do not run and Node.js is an extra dependency.

## Examples

Bad:

```gn
action("generate_foo") {
  script = "generate_foo.sh"
}
```

Good:

```gn
action("generate_foo") {
  script = "generate_foo.py"
}
```
