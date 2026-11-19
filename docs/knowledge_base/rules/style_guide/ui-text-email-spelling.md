---
name: brave/ui-text-email-spelling
description: User-facing text spells "email" without a hyphen, never "e-mail".
rubric: readability
severity: nit
---

**Rule.** Write "email", NEVER "e-mail".

**Why.** One spelling keeps strings consistent; "email" is the current standard.

## Examples

Bad:

```xml
<message name="IDS_ACCOUNT_EMAIL" desc="Label of the account email field">
  E-mail address
</message>
```

Good:

```xml
<message name="IDS_ACCOUNT_EMAIL" desc="Label of the account email field">
  Email address
</message>
```
