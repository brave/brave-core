# Commit messages

How to write the message of a brave-core commit. This adapts Chromium's CL
description guide (`../agents/shared/skills/cl-description/cl-description.md`)
to brave-core; where the two differ, follow this file.

## Committing

- Commit with `git cr commit --format`. It wraps the body at 72 columns, as
  Gerrit's "Format" button does, and refuses a subject that is over 72 columns
  once the hook has added its tags. Don't wrap by hand: write each paragraph as
  one line, with one `-m` per paragraph or with `-F <file>`.
- The hook writes the `[cr<N>]` and `--tagged` prefixes, the "Chromium changes:"
  block (`--culprit`), the "Resolves" links (`--issue`) and the `Change-Id`.
  Don't write any of them yourself.
- `--format` leaves the subject, indented paragraphs and the "Chromium changes:"
  block as they are. It also leaves a line that starts like a trailer
  (`<Word>:`) on one line, so don't start a paragraph that way.

## Subject

- `[Component] Summary`, with no colon after the bracket, and under 50 columns
  if possible.
- Wrap code names in backticks: classes, functions, features, files, GN targets,
  flags and commands. Write "Pin the `kTtc` default", not "Pin the kTtc
  default".

## Body

The message is the change's first impression, on its reviewers and on whoever
digs through the history later. It gives the high-level view at a glance, and
points to what a deeper look needs: the upstream change, an issue, a design doc.
A reference alone isn't enough: say what changes and why.

- Leave the body out when the subject says enough.
- Give the problem and the fix, usually in one sentence each. Name code only
  where the explanation needs it.
- Describe the change as it stands, not how it came about: no anecdotes about
  what was tried first, which build or run found it, or what a review said.
- Keep it succinct, and don't claim anything that wasn't verified.
- Refer to an upstream Chromium change by its CL, `crrev.com/c/<number>` (the
  commit's `Reviewed-on:` trailer), never by its hash:
  `git -C .. log --grep='c/chromium/src/+/<number>'` finds the commit from it.
  The "Chromium changes:" block keeps the culprit's hash.
- Use an indented paragraph for quoted output or code: it is kept as it is.
- No markdown-style `[text](url)` links.

## Footer

- No `Bug:` line: brave-core links issues through `--issue`.
- No `Co-Authored-By`, attribution or other trailers, and no `RELNOTES`,
  `TESTED` or `Test:` lines.
- Keep a `Change-Id` the message already has.

## Fixing a committed message

Add an `amend!` commit with `git cr commit --format --allow-empty -F <file>`.
The file holds `amend! <old subject>`, a blank line, then the whole new message:
its prefixes, and the original's "Chromium changes:" block copied from
`git log -1 --format=%B <commit>` with its indentation, but not its `Change-Id`.
`brockit rebase` keeps the target's `Change-Id` when it squashes the `amend!`.
