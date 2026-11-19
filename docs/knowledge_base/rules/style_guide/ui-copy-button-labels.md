---
name: brave/ui-copy-button-labels
description:
  Button labels have 3 words or fewer, lead with a verb describing the action,
  use sentence case, and omit articles.
rubric: readability
severity: should_fix
---

**Rule.** Keep a button label to 3 words or fewer. Lead with a verb that
describes what the button does, use sentence case (NEVER all caps), and leave
out articles (the, a, an).

**Why.** A short verb label tells users what happens when they press it, and
fits once translated.

## Examples

Bad:

```xml
<message name="IDS_WALLET_BACKUP_BUTTON" desc="Button label in the wallet backup prompt">
  Back Up The Wallet Now
</message>
```

Good:

```xml
<message name="IDS_WALLET_BACKUP_BUTTON" desc="Button label in the wallet backup prompt">
  Back up wallet
</message>
```
