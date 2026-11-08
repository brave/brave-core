---
name: brave/ui-text-colon-capitalization
description:
  In user-facing text, the word after a colon is capitalized only when a
  complete sentence follows.
rubric: readability
severity: nit
---

**Rule.** Capitalize the word after a colon only when what follows is a complete
sentence. After a colon introducing a word, phrase or list, continue in
lowercase.

**Why.** A capital after a colon signals a new sentence; using it otherwise
misleads readers and translators about the structure.

## Examples

Bad:

```xml
<message name="IDS_BLOCKED_TYPES" desc="Summary of the blocked content types">
  Blocked: Trackers, ads, and scripts
</message>
```

Good:

```xml
<message name="IDS_BLOCKED_TYPES" desc="Summary of the blocked content types">
  Blocked: trackers, ads, and scripts
</message>
```
