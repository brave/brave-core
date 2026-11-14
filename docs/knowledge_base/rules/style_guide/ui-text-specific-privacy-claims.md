---
name: brave/ui-text-specific-privacy-claims
description:
  User-facing text calling something "private" or "secure" says specifically
  what is private or secured, how, and who has access.
rubric: correctness
severity: should_fix
---

**Rule.** When a string claims something is private or secure, say specifically
what is private or secured, how, and, for privacy, who has access. Do not make
vague claims such as "Your data is private" or "Browse securely".

**Why.** Vague privacy and security claims create legal and trust risks: users
read them as promises Brave may not be able to keep.

## Examples

Bad:

```xml
<message name="IDS_SYNC_PRIVACY_NOTE" desc="Privacy note in sync settings">
  Your synced data is private and secure.
</message>
```

Good:

```xml
<message name="IDS_SYNC_PRIVACY_NOTE" desc="Privacy note in sync settings">
  Your synced data is encrypted on your device. Only devices in your sync chain can read it.
</message>
```
