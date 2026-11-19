---
name: brave/vlog-component-name-matches-directory
description:
  "The component name in VLOG messages matches the name of the component's
  directory."
rubric: readability
severity: nit
---

**Rule.** When a `VLOG` message names the component it comes from, use the name
of the component's directory (`policy` for `components/policy`, `brave_policy`
for `brave/components/brave_policy`), not a nickname or an abbreviation.

**Why.** Developers filter logs by component, and pick the name to search for
from the directory they are working in.

## Examples

Bad:

```cpp
// In brave/components/brave_policy/.
VLOG(1) << "BravePol: policy loaded";
```

Good:

```cpp
VLOG(1) << "brave_policy: policy loaded";
```
