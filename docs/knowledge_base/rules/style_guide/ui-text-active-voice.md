---
name: brave/ui-text-active-voice
description:
  User-facing strings are written in active voice, saying who does something
  rather than what was done by whom.
rubric: readability
severity: should_fix
---

**Rule.** Write user-facing text in active voice: name who or what acts (Brave,
the page, you) and put it first. Rewrite passive constructions such as "has been
cleared by", "was blocked" or "is required".

**Why.** Brave's voice is concise, descriptive, simple and active. Passive
sentences are longer, hide who acted, and are harder to translate.

## Examples

Bad:

```xml
<message name="IDS_HISTORY_CLEARED" desc="Toast shown after history is cleared">
  Your browsing history has been cleared by the system.
</message>
```

Good:

```xml
<message name="IDS_HISTORY_CLEARED" desc="Toast shown after history is cleared">
  Brave cleared your browsing history.
</message>
```
