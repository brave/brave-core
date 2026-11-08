---
name: brave/comments-make-sense-to-future-readers
description:
  'Comments describe the code as it is, for a reader who never saw the change:
  no history, no removed code, and no justification the code already makes
  evident.'
rubric: readability
severity: should_fix
---

**Rule.** A comment MUST make sense to someone reading the code for the first
time, with no knowledge of the pull request. Do not add comments that:

- mention removed code, earlier behaviour, or the change itself ("previously",
  "no longer", "changed from", "per review feedback");
- narrate how the code moved between classes or commits, even within the same
  pull request;
- justify the structure against an alternative the reader never saw, when the
  finished code is self-explanatory.

Keep a comment only when it explains a non-obvious constraint the code cannot
express (an ordering requirement, a workaround for an upstream bug).

**Why.** Comments are part of the code base, not a changelog: history belongs in
version control. Reasoning left behind from a refactor only makes sense to
someone who knows the abandoned design, and confuses everyone else.

## Examples

Bad:

```cpp
// Previously this used a raw pointer, now using unique_ptr.
// The timeout parameter was removed since it's no longer needed.
int ProcessRequest(const GURL& url);

// Password import is handled by BraveExternalProcessImporterHost in the
// browser process, alongside the utility-process import, so it participates
// in the same import lifecycle.
StartPasswordImport();
```

Good:

```cpp
// Processes the request synchronously. Returns the HTTP status code.
int ProcessRequest(const GURL& url);

StartPasswordImport();
```
