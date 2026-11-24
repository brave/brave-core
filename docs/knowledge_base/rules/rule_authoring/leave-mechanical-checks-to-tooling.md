---
name: brave/rule_authoring/leave-mechanical-checks-to-tooling
rubric: design
severity: should_fix
---

**Rule.** Don't add a review rule, or widen one, for what a tool can check
deterministically: compiler errors and warnings, `static_assert`s, clang
plugins, `gn check` and DEPS include rules, presubmit checks, linters and
formatters, tests, or `knowledge_base.py lint` and the code health scripts. If a
check already covers it, rely on that; if none does, add or extend the check (a
presubmit, a lint rule, a test) and name it in the change instead. Keep rules
for what needs judgement: intent, design, whether a behaviour is right. A rule
whose whole check is a pattern over the added lines with one fixed answer is a
presubmit; flag it, and ask for the tool.

**Why.** A tool checks every change the same way, fails before review, and
doesn't miss a case. A rule costs reviewer and agent context on every change it
matches, is applied unevenly, and drifts from the tool it duplicates. Rules that
restate tooling crowd out the judgement calls that reviews are for.
