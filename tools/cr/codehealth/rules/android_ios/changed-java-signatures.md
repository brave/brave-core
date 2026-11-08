---
name: android_ios/changed-java-signatures
rubric: android_ios
severity: regression
---

**Rule.** Brave Java that overrides, calls or subclasses an upstream method or
constructor whose signature changed in the range follows the new signature.

**Why.** The Linux build never compiles Android Java, so a lost parameter or a
removed helper only breaks the Android build, or worse, a Brave `@Override` that
no longer overrides anything.

**How to check.** For each candidate commit, take the method names declared on
both `-` and `+` lines (changed signatures), and those on `-` lines only
(renames and removed helpers), and grep Brave's Java, patches and plasters for
them. Grep all of Brave's Java, not only `android/`: subclasses and backports
also live under `browser/`. For constructors of classes Brave subclasses or
redirects with a bytecode adapter, compare the parameter list with the Brave
subclass's `super(...)` call. A `new Brave\1` plaster passes upstream's new
arguments along while the Brave constructor keeps the old list.

## Examples

- M157: `UrlBarCoordinator.setKeyboardVisibility()` lost a parameter.
- M157, 8093.0: `shouldIncludeHomeButtonIfEnabled()`, removed with its
  `keep_home_button_in_toolbar` param, left Brave's `@Override` of
  `createActionConfigs` uncompilable.
- M157, 8093.0: `AutocompleteMediator` gained `urlTextWrappingSupplier`; 8097.1:
  `TopToolbarCoordinator` changed again after the rebase had fixed it.
- M158, 8111.0: `BraveBottomBarCoordinator`, behind a `new Brave\1` plaster.
- M157, 8097.1: `BraveBottomControlsMediator`, and the backported `ExportFlow`
  using removed histogram helpers, both under `browser/`.
