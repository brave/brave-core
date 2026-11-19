---
name: brave/docs-link-to-canonical-sources
description:
  General project documentation links to the canonical source, with a brief
  Brave-specific note, rather than duplicating it.
rubric: readability
severity: should_fix
---

**Rule.** In general project documentation, link to the canonical source (such
as Chromium's documentation) instead of copying it, and add only a brief note
with the Brave-specific context. This does NOT apply to entries in
`docs/best-practices/*.md`, which are self-contained.

**Why.** Duplicated documentation inevitably drifts out of sync with its source,
and then contradicts it.

## Examples

Bad:

```markdown
## How to Add a Feature Flag

1. Create a feature in features.h...
2. Register in about_flags.cc...
```

Good:

```markdown
## How to Add a Feature Flag

Follow the
[Chromium feature flag guide](https://chromium.googlesource.com/chromium/src/+/main/docs/how_to_add_your_feature_flag.md).
Brave-specific: also register in `brave/browser/about_flags.cc`.
```
