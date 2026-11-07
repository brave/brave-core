---
name: overlap/overridden-files
rubric: overlap
severity: regression
---

**Rule.** Every range commit touching an upstream file Brave overrides
(`chromium_src`, a patch, a plaster or a Lit mangler) leaves Brave's override
applying, on the path that matters.

**Why.** Most real breaks are semantic: the tree compiles and the override's
anchor still matches, but upstream moved what it hooks. This list is what the
other overlap rules read; each names the mechanism to look for.

**How to check.** Group the candidates by overridden file, so that each override
is read once against every commit that touches it. For each commit, read only
the Brave-relevant part: `git show <hash> -- <overridden paths>`. In an
incremental range, the net `git diff -U3 $A $B -- <path>` per overridden file is
cheaper and reads the same. Two mechanical checks are cheap: every non-`BRAVE_`
`#define` token still appears in the pristine upstream file (see
`define-reach`), and the identifiers on each diff's `+`/`-` lines intersect
Brave's anchors.

## Examples

- M158, 8115.1: the net diff per overridden file was 179 files, 8k lines.
