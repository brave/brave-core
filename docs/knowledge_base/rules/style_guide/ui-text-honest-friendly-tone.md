---
name: brave/ui-text-honest-friendly-tone
description:
  User-facing text is honest and friendly, but never jokey or whimsical.
rubric: readability
severity: should_fix
---

**Rule.** Write user-facing text in an honest, friendly tone. Do not use jokes,
puns, cute phrasing or whimsy, and do not soften or overstate what happened.

**Why.** Jokes read badly when something has gone wrong, wear thin on repeat,
and rarely survive translation. Honest wording keeps users' trust.

## Examples

Bad:

```xml
<message name="IDS_IMPORT_FAILED" desc="Error shown when bookmark import fails">
  Uh-oh, our bookmark gremlins ate your import!
</message>
```

Good:

```xml
<message name="IDS_IMPORT_FAILED" desc="Error shown when bookmark import fails">
  Brave couldn't import your bookmarks. Try again.
</message>
```
