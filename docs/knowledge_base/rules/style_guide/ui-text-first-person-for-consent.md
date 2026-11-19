---
name: brave/ui-text-first-person-for-consent
description:
  Strings in which the user takes an action or gives consent (such as checkbox
  labels) use the first person (I, me, my).
rubric: readability
severity: should_fix
---

**Rule.** When a string is what the user says by acting, such as a consent
checkbox or an agreement button, write it in the first person: "I've read...",
"I agree...". Everywhere else, address the reader as "you".

**Why.** A consent statement is the user's own statement. Phrasing it in the
first person makes clear who is agreeing to what.

## Examples

Bad:

```xml
<message name="IDS_TOS_CHECKBOX" desc="Label of the terms of service consent checkbox">
  You've read the Brave terms of service
</message>
```

Good:

```xml
<message name="IDS_TOS_CHECKBOX" desc="Label of the terms of service consent checkbox">
  I've read the Brave terms of service
</message>
```
