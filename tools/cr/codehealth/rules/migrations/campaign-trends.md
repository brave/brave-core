---
name: migrations/campaign-trends
rubric: migrations
severity: migration
---

**Rule.** Brave follows the old → new idiom of each active upstream campaign in
the range, and stays off idioms upstream is retiring.

**Why.** A campaign ends with the old idiom removed or banned, and Brave code
still on it then breaks the build, or carries a growing migration debt every
lift.

**How to check.** The candidate list is `campaigns.py`'s clusters: commits
sharing a `Bug:` id, mostly campaign verbs ("Migrate X to Y", "Replace",
"Convert", `[dcheck-to-check]`). For each cluster, read a representative diff
and write down the old → new idiom. Measure the trend with
`git grep -c '<old idiom>'` at both tags: a falling count means an active
campaign (state the command with the numbers); a flat count means a stale
deprecation that isn't worth an item. Headers that say "Do not add more public
accessors" or carry a `TODO(crbug)` confirm the direction. Then grep Brave,
including `browser/resources/**/br/`, `rewrite/` and platform-only sources.
Report the count, the list of sites (or the grep command when there are many)
and one upstream example of the new idiom. Skip ash/ChromeOS, roll, gardening
and large feature-bug clusters (Glic, Actor) unless Brave touches the code.

## Examples

- `CreateForWebContents(` lines in `chrome/browser/ui/tab_helpers.cc`: 111 at
  155, 36 at 157.0.8090.1, 24 at 157.0.8093.0.
