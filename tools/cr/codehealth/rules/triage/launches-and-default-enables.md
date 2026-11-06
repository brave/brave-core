---
name: triage/launches-and-default-enables
rubric: triage
severity: regression
---

**Rule.** Launches, "Enable X by default" and "Default-enable X" subjects get
checked against what Brave disables, hides or pins.

**Why.** A launch can bring back a feature Brave turned off, through a new entry
point or a new feature name that Brave's pins don't cover.

**How to check.** For each candidate, find the feature it enables and look for
Brave's disablement of it, or of the UI it replaces, as in sweep
[b](../../SKILL.md#b-brave-disablements-and-defaults). A launch Brave neither
disables nor wants to keep is a privacy review entry.

## Examples

- M159, 8148.0:
  [ac7e11ee8115](https://chromium.googlesource.com/chromium/src/+/ac7e11ee8115fd25741b6d225bb0fd70dcddcb85):
  a "Default-enable" subject
