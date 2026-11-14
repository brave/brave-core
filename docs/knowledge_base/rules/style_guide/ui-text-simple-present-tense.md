---
name: brave/ui-text-simple-present-tense
description:
  User-facing strings default to the simple present tense, avoiding future,
  progressive (-ing) and other complex verb forms.
rubric: readability
severity: nit
---

**Rule.** Write verbs in the simple present tense unless that changes the
meaning. A verb preceded by was/were, has/have, is/are, be or will, or ending in
-ing, is not simple tense: rewrite "Clicking X will show..." as "Click X to
see...".

**Why.** Simple present is the shortest and clearest form, and the easiest to
translate consistently.

## Examples

Bad:

```xml
<message name="IDS_SEARCH_DEFAULT_HINT" desc="Hint text in search settings">
  Clicking "Search default" will show a list of search options
</message>
```

Good:

```xml
<message name="IDS_SEARCH_DEFAULT_HINT" desc="Hint text in search settings">
  Click "Search default" to see a list of search options
</message>
```
