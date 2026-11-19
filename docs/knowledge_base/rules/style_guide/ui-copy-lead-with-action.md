---
name: brave/ui-copy-lead-with-action
description:
  UI copy starts with an imperative verb or the most important information,
  omitting lead-ins such as "Be sure to" or "Would you like to".
rubric: readability
severity: should_fix
---

**Rule.** Start UI copy with the imperative verb or the most important
information, and drop extraneous words: "Select the address...", not "Be sure to
select the address..."; "Delete history?", not "Would you like to delete your
history now?"; "Create an account", not "You can create an account".

**Why.** Translations may be truncated, so whatever comes first must carry the
meaning. Lead-ins add length and push the action out of view.

## Examples

Bad:

```xml
<message name="IDS_WALLET_ADDRESS_HINT" desc="Hint under the address field">
  Be sure to select the address that matches your cryptocurrency type.
</message>
```

Good:

```xml
<message name="IDS_WALLET_ADDRESS_HINT" desc="Hint under the address field">
  Select the address that matches your cryptocurrency type.
</message>
```
