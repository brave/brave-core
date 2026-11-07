---
name: triage/launches-and-default-enables
rubric: triage
severity: regression
---

**Rule.** Launches and "Enable X by default" subjects get checked against what
Brave disables, hides or pins.

**Why.** A launch can bring back a feature Brave turned off, through a new entry
point or a new feature name that Brave's pins don't cover.

**How to check.** For each candidate, find the feature it enables and look for
Brave's disablement of it, or of the UI it replaces, as in sweep
[b](../../SKILL.md#b-brave-disablements-and-defaults). A launch Brave neither
disables nor wants to keep is a privacy review entry.
