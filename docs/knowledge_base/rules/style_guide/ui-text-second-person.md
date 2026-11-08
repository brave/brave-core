---
name: brave/ui-text-second-person
description:
  User-facing strings address the reader in the second person (you, your), not
  as "the user".
rubric: readability
severity: should_fix
---

**Rule.** Address the reader directly as "you" and "your". Do not refer to the
reader in the third person ("the user", "users", "his or her").

**Why.** Second person is direct and friendly, and it keeps strings short.

## Examples

Bad:

```xml
<message name="IDS_ACCOUNT_LOGIN_TITLE" desc="Title of the account login dialog">
  The user must log in to their Brave account
</message>
```

Good:

```xml
<message name="IDS_ACCOUNT_LOGIN_TITLE" desc="Title of the account login dialog">
  Log in to your Brave account
</message>
```
