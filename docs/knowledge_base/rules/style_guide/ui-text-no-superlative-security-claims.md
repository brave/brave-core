---
name: brave/ui-text-no-superlative-security-claims
description:
  User-facing text must never make superlative privacy or security claims such
  as "military-grade encryption", "100% anonymous" or "unhackable".
rubric: correctness
severity: blocking
---

**Rule.** NEVER use superlative privacy or security claims: "military-grade" or
"unbreakable" encryption, "completely private", "100% anonymous", "totally
secure", "unhackable", "hacker-proof", or any other superlative. Describe the
specific protection instead.

**Why.** No product can guarantee these; superlative claims create legal
exposure and break users' trust.

## Examples

Bad:

```xml
<message name="IDS_VPN_HEADLINE" desc="Headline of the VPN promotion">
  Military-grade encryption keeps you 100% anonymous.
</message>
```

Good:

```xml
<message name="IDS_VPN_HEADLINE" desc="Headline of the VPN promotion">
  Brave VPN encrypts your connection on any network.
</message>
```
