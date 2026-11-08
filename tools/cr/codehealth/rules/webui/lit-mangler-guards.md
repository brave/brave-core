---
name: webui/lit-mangler-guards
rubric: webui
severity: regression
---

**Rule.** Each lit_mangler still finds what it changes in the template at the
new tag.

**Why.** `mangle()` throws when its template is missing, which is loud.
`mangleAll`, `?.` and `if (el)` guards skip silently when upstream renames or
moves the node.

**How to check.** The candidates change templates Brave mangles. For each, check
the mangler's `mangleAll`, `?.` and `if (el)` targets against the template at
the new tag.
