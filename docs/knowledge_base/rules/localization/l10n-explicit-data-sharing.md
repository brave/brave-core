---
name: brave/l10n-explicit-data-sharing
description:
  Strings for actions that share data say explicitly where the data goes, as in
  "send your tabs to Brave AI".
rubric: readability
severity: should_fix
---

**Rule.** When a string describes sending or sharing data, say where it goes:
"send your tabs to Brave AI", not just "send your tabs".

**Why.** Users decide whether to share data based on where it goes; leaving the
destination out hides what they agree to.

## Examples

Bad:

```xml
<message name="IDS_AI_CHAT_SHARE_TABS" desc="Toggle label in the Leo context menu">
  Send your tabs
</message>
```

Good:

```xml
<message name="IDS_AI_CHAT_SHARE_TABS" desc="Toggle label in the Leo context menu">
  Send your tabs to Brave AI
</message>
```
