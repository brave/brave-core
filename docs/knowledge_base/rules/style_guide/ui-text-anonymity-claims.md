---
name: brave/ui-text-anonymity-claims
description:
  User-facing text almost never says "anonymous" or "anonymity", and any such
  claim needs privacy team review.
rubric: correctness
severity: blocking
---

**Rule.** Do not describe anything as "anonymous" or as providing "anonymity"
unless it is practically impossible for any adversary, Brave included, to
identify the person, and the wording has privacy team review. Say "IP dropping
server" or "proxy server", not "anonymizing server". Brave Search's "anonymous
community contributions" is an approved message.

**Why.** An anonymity claim that does not hold is a privacy problem as well as a
legal and trust risk.

## Examples

Bad:

```xml
<message name="IDS_WDP_DESCRIPTION" desc="Description of the Web Discovery Project setting">
  Your data is sent through an anonymizing server, so you stay anonymous.
</message>
```

Good:

```xml
<message name="IDS_WDP_DESCRIPTION" desc="Description of the Web Discovery Project setting">
  Your data is sent through an IP dropping server, so Brave can't see your IP address.
</message>
```
