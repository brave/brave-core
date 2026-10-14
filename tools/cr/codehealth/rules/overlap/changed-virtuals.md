---
name: overlap/changed-virtuals
rubric: overlap
severity: regression
---

**Rule.** Brave's overrides of upstream virtuals still override something that
is called, and Brave uses the new hooks meant for its restrictions.

**Why.** Removed or split virtuals leave a Brave override dead. A new virtual
can be a hook that Brave's equivalent restriction should use, or a dedicated
observer method that takes over from a generic one Brave listens to.

**How to check.** Diff the virtuals of `ContentBrowserClient`,
`ExtensionsClient`, `AutofillClient`, `Configurator` and other base classes
Brave overrides, at both tags. For each removed or re-signed one, find Brave's
override; for each new one, ask whether a Brave restriction belongs there.

## Examples

- `ExtensionsClient::IsCapturableURL`, a hook Brave's restriction should use.
- M157, 8094.1: `OnTabBlockedStateChanged` replaced
  `OnTabChangedAt(kBlockedOnly)`, which `SharedPinnedTabService` relied on.
