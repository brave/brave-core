---
name: brave/ui-text-short-sentences
description:
  Sentences in user-facing strings aim for 15 words or fewer; a longer one is
  split in two or into bullet points.
rubric: readability
severity: nit
---

**Rule.** Keep each sentence of user-facing text to about 15 words or fewer.
Split a longer sentence in two, move a question into a title and its consequence
into the body, or use bullet points. Read the text aloud to check it.

**Why.** Short sentences are read at a glance, survive the text expansion of
translation, and are easier to translate.

## Examples

Bad:

```xml
<message name="IDS_SHIELDS_DOWN_CONFIRM" desc="Body of the Shields down confirmation">
  Are you sure you want to put Shields down for this page? Shields down may reduce your privacy.
</message>
```

Good:

```xml
<message name="IDS_SHIELDS_DOWN_CONFIRM_TITLE" desc="Title of the Shields down confirmation">
  Put Shields down?
</message>
<message name="IDS_SHIELDS_DOWN_CONFIRM_BODY" desc="Body of the Shields down confirmation">
  Shields down may reduce your privacy on this site.
</message>
```
