---
name: brave/ui-text-error-messages-tone
description:
  Error messages avoid exclamations, apologies, "please", blame, technical
  jargon, empty promises and new concepts.
rubric: readability
severity: should_fix
---

**Rule.** In error messages, do not use exclamations ("Whoopsie!"), apologies
("Sorry"), "please", blame of the user, technical jargon (server, fetch, abort,
illegal, cache, invalid), empty promises, or concepts the user has not met
before.

**Why.** Errors are read by someone already frustrated: jargon and blame make it
worse, and apologies and promises add length without helping.

## Examples

Bad:

```xml
<message name="IDS_PAGE_FETCH_ERROR" desc="Error shown when a page fails to load">
  Whoopsie! Sorry, the server aborted the fetch. Please try again later.
</message>
```

Good:

```xml
<message name="IDS_PAGE_FETCH_ERROR" desc="Error shown when a page fails to load">
  This page failed to load. Try reloading, or setting Shields down.
</message>
```
