# Vendored skill — remove after a Chromium roll includes it

This is a verbatim mirror of upstream Chromium's
`browser-window-feature-refactor` skill from:

    https://chromium-review.googlesource.com/c/chromium/src/+/7991656
    [bedrock] Add BrowserWindowFeatures refactor skill

That CL landed upstream _after_ the Chromium version this branch pins, so the
skill is not yet present in `src/agents/skills/`. It is vendored here so it is
usable on the Brave codebase now (see `agents/skills/setup.py`, which links both
Brave and upstream skills into `.claude/skills/`).

Brave's skills take precedence over upstream ones of the same name, so **once a
Chromium roll brings this skill into
`src/agents/skills/browser-window-feature-refactor/`, delete this vendored
copy** — otherwise it will keep shadowing the (newer) upstream version. Only
`SKILL.md` and `references/bwf-ordering.md` are mirrored; upstream's `OWNERS` is
Chromium-specific and intentionally omitted.
