---
name: brave/ui-copy-confirmation-modals
description:
  A confirmation modal's title states the action as a question, its body states
  the consequences, and its button repeats the action verb.
rubric: readability
severity: should_fix
---

**Rule.** Write confirmation modals in three parts: a title stating the action
concisely as a question ("Set Brave as your default browser?"), a body stating
its consequences concisely, and a confirm button that repeats the title's action
verb ("Set as default"), not "OK" or "Yes".

**Why.** Users often read only the title and the button. When both name the same
action, the choice is clear without reading the body.

## Examples

Bad:

```xml
<message name="IDS_DEFAULT_BROWSER_TITLE" desc="Title of the default browser confirmation">
  Default browser
</message>
<message name="IDS_DEFAULT_BROWSER_CONFIRM" desc="Confirm button of the default browser confirmation">
  OK
</message>
```

Good:

```xml
<message name="IDS_DEFAULT_BROWSER_TITLE" desc="Title of the default browser confirmation">
  Set Brave as your default browser?
</message>
<message name="IDS_DEFAULT_BROWSER_BODY" desc="Body of the default browser confirmation">
  Any web links you click will open in the Brave browser.
</message>
<message name="IDS_DEFAULT_BROWSER_CONFIRM" desc="Confirm button of the default browser confirmation">
  Set as default
</message>
```
