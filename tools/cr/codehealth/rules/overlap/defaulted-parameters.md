---
name: overlap/defaulted-parameters
rubric: overlap
severity: regression
---

**Rule.** Brave's subclasses of an upstream class pass the argument that a new
defaulted constructor or method parameter stands for, when upstream's own
subclasses do.

**Why.** A new parameter with a default compiles in every Brave caller, so the
build says nothing, but the default can change behaviour that the class used to
decide another way. Upstream updates its own callers that need the old
behaviour; Brave's keep the new default.

**How to check.** For each new defaulted parameter, read what the default does
and which upstream callers the commit changed to pass another value, then list
Brave's subclasses and callers of the class, and check whether they are like the
upstream ones that opted out.

## Examples

- M160, 8166.0: `TopChromeWebUIConfig`'s `ProcessModel` defaults to
  `kTopChromeProcess`, which put `chrome://leo-ai` in the Top Chrome process.
