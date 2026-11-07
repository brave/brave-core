---
name: enforcement/commit-body-announcements
rubric: enforcement
severity: deprecation
---

**Rule.** Brave prepares for the enforcement, deprecations and features that
upstream commit messages announce ahead of time.

**Why.** A commit that lands "in preparation for" a new check, or preempts an
upcoming clang, presubmit or plugin rule, is the only notice before the build
breaks. Features Brave builds on show up the same way.

**How to check.** The candidates' bodies match
`preempt|upcoming (clang|presubmit|plugin)|will be disallowed|\bLSC\b|is being deprecated|in preparation (for|of)`.
Read what each one prepares for, and grep Brave for code the coming change
affects.

## Examples

- M157, 8097.1: History Search AI, which Brave's history search builds on.
- M157, 8103.1: Lit template structure rules ahead of an ESLint check.
