---
name: brave/disable-controls-during-async-operations
description:
  A button, preference or other control that starts an async operation must be
  disabled until the operation's callback runs, and re-enabled there.
rubric: correctness
severity: should_fix
---

**Rule.** When a click on a button, preference or other interactive element
starts an asynchronous operation, the handler MUST disable that element (for
example `setEnabled(false)`) before starting it, and the operation's completion
callback MUST re-enable it.

**Why.** A control that stays enabled while its operation runs can be clicked
again, and starts the operation twice: a second email sent, a second request
racing the first.

## Examples

Bad:

```java
preference.setOnPreferenceClickListener(pref -> {
    accountService.resendConfirmationEmail(callback);
    return true;
});
```

Good:

```java
preference.setOnPreferenceClickListener(pref -> {
    preference.setEnabled(false);
    accountService.resendConfirmationEmail(result -> {
        preference.setEnabled(true);
        // handle result
    });
    return true;
});
```
