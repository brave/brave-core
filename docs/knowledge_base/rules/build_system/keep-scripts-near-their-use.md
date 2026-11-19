---
name: brave/keep-scripts-near-their-use
description:
  'A new build or utility script lives next to the GN target or DEPS
  configuration that runs it, not in the shared brave/script directory.'
rubric: design
severity: should_fix
---

**Rule.** Put a new script in the directory of the GN `action()` that runs it,
or near the dependency or configuration a `DEPS` hook uses it for. Do not add
new build or utility scripts to `brave/script`: keep that directory for the
scripts already there.

**Why.** A script next to its only user is found, reviewed and deleted with it.
A shared `brave/script` directory hides which target needs a script, so it grows
with scripts nobody can safely remove.

## Examples

Bad:

```gn
action("generate_foo_list") {
  script = "//brave/script/generate_foo_list.py"
}
```

Good:

```gn
# brave/components/foo/BUILD.gn
action("generate_foo_list") {
  script = "generate_foo_list.py"
}
```
