---
name: webui/lit-controls-take-only-pref-key
rubric: webui
severity: regression
---

**Rule.** Brave binds no `pref=` on a control that is on
`PrefKeyObserverMixinLit` at the new tag, and lists its `pref=` bindings on a
control upstream announces it will migrate.

**Why.** The mixin asserts on a `pref` binding, so the control throws. Commits
that move upstream's clients of a Polymer control to `prefKey` "in preparation
for migrating" it announce the next one, which usually follows within a lift or
two.

**How to check.** List the controls on the mixin at the new tag
(`git grep -l 'PrefKeyObserverMixinLit(' $B -- chrome/browser/resources`), and
grep Brave's templates for a `pref=` binding on each tag, including attributes
on later lines. For a candidate that announces a migration, list Brave's `pref=`
bindings on that tag as a deprecation.

## Examples

- M158, 8113.1: the Tor bridges radios bound a fake pref.
- M158: announced at 8117.1, and at 8127.2 `settings-toggle-button` went Lit, so
  17 Brave toggles throw.
