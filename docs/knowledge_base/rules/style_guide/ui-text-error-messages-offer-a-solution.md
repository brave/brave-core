---
name: brave/ui-text-error-messages-offer-a-solution
description:
  An error message says what happened and gives a solution or a way to contact
  support, never leaving the user at a dead end.
rubric: readability
severity: should_fix
---

**Rule.** Every error message says what happened AND what the user can do about
it: a step to try, or a way to contact support. An error message MUST NOT be a
dead end.

**Why.** An error that only reports failure leaves users stuck, and turns a
recoverable problem into an abandoned task or a support request.

## Examples

Bad:

```xml
<message name="IDS_SEARCH_NO_RESULTS" desc="Message shown when a search has no results">
  No results.
</message>
```

Good:

```xml
<message name="IDS_SEARCH_NO_RESULTS" desc="Message shown when a search has no results">
  Zero results for your query. Try a new query with different keywords.
</message>
```
