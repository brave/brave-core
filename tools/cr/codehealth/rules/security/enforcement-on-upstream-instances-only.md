---
name: security/enforcement-on-upstream-instances-only
rubric: security
severity: regression
---

**Rule.** When upstream enforces something and updates its own instances to
comply, Brave's instances of the same base class, element or data source comply
too.

**Why.** Commits that "enforce X" and "update the relevant sources to declare Y"
fix only upstream's subclasses. Brave's stay as they were, and either break or
keep the hole open. Brave subclasses also inherit upstream's new restrictive
overrides.

**How to check.** For each candidate commit, find the base class or element it
enforces on, list Brave's subclasses or uses of it, and check each one. For "add
X to the remaining N uses of `<element>`" in WebUI, grep Brave's templates for
that element without the attribute. For WebUI, build a table of requesting page
× target host from Brave's CSP overrides and the `//host` URLs in the front-end
code.

## Examples

- `WebUIURLLoaderFactory` now blocks loads between `chrome-untrusted://` hosts,
  but no Brave data source allows its embedders.
- `PlaylistDataSource : FaviconSource` inherits upstream's new restrictive
  overrides.
- M158, 8115.1: `route-path` on `<settings-subpage>`, which sets the tab title.
