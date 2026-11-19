---
name: brave/ui-text-learn-more-with-context
description:
  A "Learn more" link says what there is to learn about, as in "Learn more about
  Brave Rewards".
rubric: readability
severity: nit
---

**Rule.** Give "Learn more" links context: "Learn more about Brave Rewards", not
a bare "Learn more", unless the surrounding text makes the subject unambiguous.

**Why.** A bare "Learn more" does not tell screen reader users, or anyone
scanning, where the link goes.

## Examples

Bad:

```xml
<message name="IDS_REWARDS_LEARN_MORE" desc="Link at the bottom of the Rewards panel">
  Learn more
</message>
```

Good:

```xml
<message name="IDS_REWARDS_LEARN_MORE" desc="Link at the bottom of the Rewards panel">
  Learn more about Brave Rewards
</message>
```
