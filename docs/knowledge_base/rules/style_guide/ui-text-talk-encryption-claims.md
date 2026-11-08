---
name: brave/ui-text-talk-encryption-claims
description:
  Copy about Brave Talk must never call it "E2E" or "end-to-end" encrypted.
rubric: correctness
severity: blocking
---

**Rule.** NEVER describe Brave Talk as "E2E" or "end-to-end" encrypted. The
approved messages are free video calls for up to 4 people, no time limit, no
tracking, and no extension, app or account needed.

**Why.** An end-to-end encryption claim is a security claim users rely on;
making it without the guarantee misleads them.

## Examples

Bad:

```xml
<message name="IDS_TALK_PROMO" desc="Brave Talk promotion on the new tab page">
  Private, end-to-end encrypted video calls.
</message>
```

Good:

```xml
<message name="IDS_TALK_PROMO" desc="Brave Talk promotion on the new tab page">
  Free video calls for up to 4 people, with no time limit.
</message>
```
