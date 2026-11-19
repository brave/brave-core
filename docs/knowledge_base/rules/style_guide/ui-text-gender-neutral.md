---
name: brave/ui-text-gender-neutral
description:
  'User-facing strings use gender-neutral language: no gendered pronouns (or
  they/their when unavoidable) and no gender-latent words such as "policeman".'
rubric: readability
severity: should_fix
---

**Rule.** Rephrase to avoid gendered pronouns (he, she, his, her). When a
pronoun cannot be avoided, use "they" and "their". Replace gender-latent words
with neutral ones: "police officer", not "policeman"; "flight attendant", not
"stewardess".

**Why.** Gendered language excludes readers, and neutral wording is usually
shorter as well.

## Examples

Bad:

```xml
<message name="IDS_SHARE_HINT" desc="Hint in the share dialog">
  Send the link to a friend so he can open it in his browser.
</message>
```

Good:

```xml
<message name="IDS_SHARE_HINT" desc="Hint in the share dialog">
  Send the link to a friend to open in their browser.
</message>
```
