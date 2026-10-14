---
name: overlap/cached-state-replaced-by-parameters
rubric: overlap
severity: regression
---

**Rule.** A Brave plaster or override doesn't read a member that upstream
stopped trusting in favour of a parameter.

**Why.** When upstream starts passing data as a parameter "because the member
can be stale", any Brave code still reading the member is stale too.

**How to check.** For each candidate, find the member it stops reading, and grep
Brave's overrides of the file for it.

## Examples

- `last_query_.field_id` in `AutofillExternalDelegate::DidAcceptSuggestion`.
