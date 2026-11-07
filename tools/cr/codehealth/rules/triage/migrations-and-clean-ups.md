---
name: triage/migrations-and-clean-ups
rubric: triage
severity: migration
---

**Rule.** Migrations, clean-ups and refactors of code Brave overrides or calls
get read for Brave code left on the old type or API.

**Why.** Upstream converts its own callers and then removes the old API; Brave's
callers and overrides are converted by nobody, and a clean-up can remove the
hook point an override relied on.

**How to check.** Group the candidates by what they migrate to (e.g. one type
across many directories), and grep Brave for the old type or API. Lit and
Polymer migrations go to sweep [d](../../SKILL.md#d-webui).
