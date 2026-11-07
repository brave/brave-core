---
name: obsolete/workarounds-added-during-lift
rubric: obsolete
severity: obsolete
---

**Rule.** A workaround or patch added during this lift is still needed at the
new tag.

**Why.** Developers lift against intermediate versions, so the upstream fix can
land later in the same range. A patch the rebase regenerated can also duplicate
work upstream now does.

**How to check.** The candidates are
`vpython3 tools/cr/codehealth/scripts/obsolete.py lift-overrides`: the overrides
the lift's commits added (`origin/master...HEAD`), and the patches whose `+`
lines they changed, each with the range commits that touched its upstream file.
Read those commits against the override. Overrides no range commit touches are
only counted: an earlier range's review read them. A regenerated patch whose `+`
lines didn't change isn't listed: sweep
[a](../../SKILL.md#a-overlap-commits-touching-what-brave-overrides) reads the
range commits touching it.

## Examples

- A Widevine dep was added a day after upstream added the same line.
- M158, 8109.1: the `html_capability_element_base.cc` patch computed the
  descriptor a second time (context-only regeneration, found by the overlap).
