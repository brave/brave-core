---
name: overlap/define-reach
rubric: overlap
severity: regression
---

**Rule.** Each token a `chromium_src` override `#define`s reaches the same
upstream code at the new tag as at the old one.

**Why.** A `#define` rewrites every occurrence of its token. A different count
in the pristine upstream file means a different reach: it rewrites code it
didn't, or matches nothing after a rename.

**How to check.** The candidate list is `define_reach.py`'s output: tokens whose
count in the pristine upstream file differs between the tags. A count of 0 at
the new tag means upstream renamed the function, and the `#define` now matches
nothing. For `Foo` → `Foo_ChromiumImpl` renames, look for upstream calls to
`Foo()` inside the same file: those skip Brave's wrapper, and a default flip can
make a dormant one live. A private method may never reach the wrapper at all,
and neither does a function in an anonymous namespace: Brave's wrapper is then a
different function that nothing calls. When upstream adds a post-processing pass
inside a function Brave extends, check whether Brave's additions land before or
after it.

## Examples

- `#define AppMenu BraveAppMenu` also rewrote `GetAppMenu()` after upstream
  moved it.
- M157, 8094.1: a new internal `FormatUrlOrigin()` call skipped Brave's scheme
  swap.
- M158, 8109.1: `CreateLocalBlockingJsonRuleList` →
  `CreateAppSpecificURLsJsonRuleList` on iOS re-enabled the rule list Brave
  empties.
- M158: `ShouldTypeExpireActively()` and `kActiveContentSettingExpiry`, a
  dormant call made live by a default flip.
- `CancelBrowserClose`, a private method that never reaches the wrapper.
- M158, 8111.0: the commander group in `BuildDefaultGroups()`, in an anonymous
  namespace, dead since M131.
- A miss, found in the rebase: accelerators got action IDs in a loop that runs
  before Brave's `GetAcceleratorList()` appends its own.
