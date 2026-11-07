---
name: android_ios/bytecode-adapters
rubric: android_ios
severity: regression
---

**Rule.** Each bytecode adapter target (class, method, field) under
`build/android/bytecode/java/org/brave/bytecode/` still resolves at the new tag,
with the descriptor Brave expects.

**Why.** A stale target never matches its map entry and never errors: the method
stays private, or the owner redirect never happens. `changeMethodOwner` matches
by name only, so a method upstream re-signs, or a static it retires for a
same-named instance method, gets redirected to a descriptor Brave lacks. Adapter
operations without a `BytecodeTest` assertion are where dead hooks accumulate.

**How to check.** For each candidate commit, look up the classes and methods it
re-declares or removes in Brave's adapters
(`git grep -n '<Name>' -- build/android/bytecode`), and compare the declaration
at both tags. A target that vanished is a dead hook.

## Examples

- M158: `UndoBarController.showUndoBar` was re-signed, and the redirect went to
  a descriptor Brave lacks.
- M158: the `OmniboxResourceProvider` statics, retired for same-named instance
  methods.
