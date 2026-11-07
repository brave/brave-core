---
name: security/hardening-closes-brave-workaround
rubric: security
severity: regression
---

**Rule.** Brave workarounds that rely on a navigation, opener or WebUI behaviour
keep working after upstream hardens it.

**Why.** Brave code commented as reverting to "legacy behaviour" around a
navigation or WebUI restriction is exposed when upstream hardens that path, and
the feature breaks silently.

**How to check.** For each candidate hardening, grep Brave's front end for
`window.open(''`, `window.opener`, `.location.href =` on a popup, `top.location`
and `parent.location`, and Brave's C++ for "legacy" comments near the code the
commit changes.

## Examples

- M158: the Trezor bridge's blank-popup opener, severed by "Protect untrusted
  WebUI from tabnabbing".
