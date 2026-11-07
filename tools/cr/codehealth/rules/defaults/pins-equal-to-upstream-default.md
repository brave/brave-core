---
name: defaults/pins-equal-to-upstream-default
rubric: defaults
severity: obsolete
---

**Rule.** A Brave override that now equals upstream's default is dropped only
when it was meant as temporary.

**Why.** A pin is allowed against upstream sliding back (`rewriters.pyl`), so it
isn't an item by itself. One added to wait out an upstream rollout is.

**How to check.** For each candidate flip of a feature Brave overrides, toward
Brave's value, read the commit that added the override
(`git log origin/master -S'<feature>'`).

## Examples

- Brave reverted `kSearchNavigationPrefetch` in M156 once upstream disabled it.
