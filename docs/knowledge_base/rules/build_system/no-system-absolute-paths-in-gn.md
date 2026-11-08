---
name: brave/no-system-absolute-paths-in-gn
description:
  'Targets in the Brave build never use system-absolute paths; rebase_path() is
  always given its new_base, usually root_build_dir.'
rubric: correctness
severity: blocking
---

**Rule.** A target that is part of the Brave build MUST NOT put a
system-absolute path in its GN: every `rebase_path()` passes the path's expected
base as `new_base`, usually `root_build_dir` for actions, or the directory the
tool changes to if it changes its working directory. NEVER use one-argument
`rebase_path()` there. A tool that requires absolute paths is run through a
wrapper script that receives relative paths and makes them absolute at run time.
Host-side tooling, and targets generating wrapper scripts or configuration only
for local developer use that never run on a remote bot worker, are exempt.

**Why.** An absolute path is specific to one checkout: it interferes with remote
build execution and fails when build artifacts are reused from a cloned or
relocated checkout.

## Examples

Bad:

```gn
args = [ "--output=" + rebase_path(output) ]
```

Good:

```gn
args = [ "--output=" + rebase_path(output, root_build_dir) ]
```
