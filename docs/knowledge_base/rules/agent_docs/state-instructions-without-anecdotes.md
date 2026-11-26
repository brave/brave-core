---
name: brave/agent_docs/state-instructions-without-anecdotes
rubric: readability
severity: should_fix
---

**Rule.** In instructions for agents (skills, the documents they read, prompts
and commands), state what to do and how to recognise when it applies. Don't
recount how an instruction came about: what a run, a review or an incident did
or got wrong, which commit or build found it, or what was tried first. An
example belongs only where it shows how to recognise the case (a name, a
message, an identifier), tagged with the milestone it comes from, e.g.
`(cr159: …)`. Flag anecdotes and asides such as "(cr159: X failed CI)", "the
last run did…" or "this was added after…", and ask for the instruction they
stand for instead.

**Why.** An agent reads every line of its instructions on every run, so an
account of the past crowds out what it needs, and it may take a one-off event
for a rule to follow. Anecdotes also go stale as the code moves on, while the
instruction they justify stays. The history of a change belongs in its commit
message and in the run's report, where it is read once.
