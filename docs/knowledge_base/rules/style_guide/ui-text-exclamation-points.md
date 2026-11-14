---
name: brave/ui-text-exclamation-points
description:
  User-facing text uses exclamation points sparingly, never more than one per
  view, post, email or article.
rubric: readability
severity: nit
---

**Rule.** Use exclamation points sparingly: NEVER more than one in a view, post,
email or article. Most strings need none.

**Why.** Exclamation points read as shouting or forced enthusiasm when overused.

## Examples

Bad:

```xml
<message name="IDS_WALLET_CREATED" desc="Title shown after a wallet is created">
  Success! Your wallet is ready!
</message>
```

Good:

```xml
<message name="IDS_WALLET_CREATED" desc="Title shown after a wallet is created">
  Your wallet is ready
</message>
```
