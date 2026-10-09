# Modularization and Circular Dependencies

Rules for splitting Brave code into small GN targets and for removing
`allow_circular_includes_from`. They follow upstream's Project Bedrock
([`//docs/chrome_browser_design_principles.md`](../../../docs/chrome_browser_design_principles.md))
and the "Remove allow_circular_includes_from" LSC
([crbug.com/556482973](https://crbug.com/556482973)). For the task list see
[brave-browser#59199](https://github.com/brave/brave-browser/issues/59199).

<a id="MOD-001"></a>

## ❌ Never Add `allow_circular_includes_from`

**Do not add entries to any `allow_circular_includes_from` list.** `//.gn`
(`allow_circular_includes_from_allowlist`) limits which targets may declare the
key; Brave's declaring targets are listed in
`//brave/build/circular_includes_brave.gni`. GN rejects a new declaring target,
but not a new entry in an existing list such as
`brave_ui_allow_circular_includes_from`, so review must reject those. Solve the
cycle with MOD-003 to MOD-006 instead.

---

<a id="MOD-002"></a>

## ✅ Every Directory Gets Its Own `BUILD.gn`

**New code goes in a feature directory with a standalone `BUILD.gn`, never in
the monolithic `//brave/browser:browser` sources (`sources.gni`),
`//brave/browser/ui:ui` or `//brave/test:*` lists.** Place it in
`//brave/components/<feature>` when it has no `//chrome` dependency, otherwise
in `//brave/browser/<feature>` (or `//brave/browser/ui/<feature>`).

See also [BS-057](./build-system.md#BS-057) and
[`gni_sources.md`](../gni_sources.md).

---

<a id="MOD-003"></a>

## ✅ Split Headers From Implementation

**A target that other modules include from is a headers target; the `.cc` files
go in `:impl`.** Consumers depend on the headers target. Only the linking
targets (`//brave/browser:core` through `brave_chrome_browser_deps`, or
`//chrome/browser:browser`) depend on `:impl`.

```gn
source_set("foo") {
  sources = [ "foo.h", "foo_factory.h" ]
  # Only deps whose types appear in these headers.
  public_deps = [ "//components/keyed_service/core" ]
}

source_set("impl") {
  sources = [ "foo.cc", "foo_factory.cc" ]
  deps = [
    ":foo",
    "//chrome/browser/profiles:profile",
  ]
}
```

Use a single target with `public = [...]` and `sources = [...]` when the module
is small and has few consumers.

- `public_deps` is transitive. List a dep there only if its types appear in the
  target's public headers.
- Name the headers target after the directory (`:foo`); use `:impl` for the
  implementation. Tests go in `:unit_tests` and `:browser_tests`.
- `:browser_tests` sets `defines = [ "HAS_OUT_OF_PROC_TEST_RUNNER" ]`.
- Keep `if (is_android)`, `if (toolkit_views)` and buildflag guards identical to
  the ones around the files before the move; guard each dep like its source.

---

<a id="MOD-004"></a>

## ✅ Break Cycles at the Include, Not in GN

**`allow_circular_includes_from = [ B ]` on target `A` requires `A` to depend on
`B`, and lets `B` include `A`'s headers against the dependency direction. To
remove the entry, delete it, then:**

| Situation                                                                                                                                                  | Fix                                                                                                                                          |
| ---------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------- |
| `A` does not transitively depend on `B`, or `B` does not include `A`                                                                                       | The entry is dead. Run `gn check --fix`.                                                                                                     |
| `B` includes a header that is in `A` only because `A` is a monolith (`brave_chrome_browser_sources`, `//chrome/browser:browser`, `//chrome/browser/ui:ui`) | Brave header: move it into its own headers target (MOD-003). Upstream header: keep the entry and record it as blocked in the tracking issue. |
| `A` includes `B` and `B` includes `A`                                                                                                                      | Real cycle. Extract the shared declarations into a third headers target, or merge the two targets.                                           |
| `A` is a `component`/`static_library` with `public`/`sources`                                                                                              | Move the sources into `A_sources`, make `A` a `group` (see the `/remove-circular-includes` skill).                                           |
| `B` includes `A`, and `A` depends on `B` without including it                                                                                              | Backwards arrow. Make `B` depend on `A`, then fix the loop `gn gen` reports by deleting the edge with no matching `#include`.                |

Verify with `gn check` on every platform the files build on, and for pure
metadata changes confirm the Ninja graph is unchanged with
`siso query graph -C <out> > baseline.jsonl` before and `modified.jsonl` after.

---

<a id="MOD-005"></a>

## ❌ No Hacks to Silence `gn check`

- Never use `check_includes = false`.
- Never use `// nogncheck`, except `// nogncheck crbug.com/40147906` for
  conditional includes GN cannot model.
- Never use `-=` on `sources`, and never list one file in two targets.
- A module that includes a header still owned by a monolith (`//chrome/browser`,
  `//chrome/test:unit_tests`) stays in the monolith until that header's owner is
  modularized.

---

<a id="MOD-006"></a>

## ✅ Break the Design Cycle With Injection or Inversion

Most cycles come from one of these. Prefer fixing the cause over relocating
files.

- **`Browser*` / `BrowserView*`**: pass what is needed (`PrefService*`,
  `TabInterface&`, `BrowserWindowInterface*`) in the constructor. Do not look up
  a window with global lookups such as
  `BrowserCollection::FindBrowserWithTab()`; use
  `tab->GetBrowserWindowInterface()`.
- **Feature state**: expose through `UnownedUserData` (`X::From(tab)`) so
  consumers depend on the headers target of `X`, not on the owner
  (`BraveTabFeatures`, `BrowserWindowFeatures`).
- **Subclassing an upstream class from `//brave`**: use the templated-subclass
  technique in [`gni_sources.md`](../gni_sources.md) so Brave does not depend on
  the Chromium target that depends back on Brave.
- **Push vs pull**: a coordinator that both queries and is called back by its
  children forms a cycle. Pick one direction.
- **Factory split**: `BrowserContextKeyedServiceFactory` getters (`.h`) may live
  in the headers target while the `.cc` that wires dependencies lives in
  `:impl`.

---

<a id="MOD-007"></a>

## ✅ Chromium Patches Shape the Graph

**Prefer `chromium_src` overrides and plaster rewrites that add a dep on a
narrow Brave headers target, not on `//brave/browser`.** An upstream file
including `brave/browser/foo.h` forces the upstream target to depend on whatever
owns that header. Put the header in a small target so the dep adds one edge
instead of a cycle.

---

<a id="MOD-008"></a>

## ✅ One Small, Mechanical CL Per Target

- One target or one tightly related group per CL (about 30 files at most).
- Do not mix a move with a behaviour change.
- When a CL leaves a target with no `allow_circular_includes_from` entries, the
  same CL removes the key from that target and the target from
  `circular_includes_brave.gni`.
- Demote transitional `public_deps` to `deps` in a follow-up, after consumers
  have direct deps. A demotion can pass `gn check` and still fail at link time;
  build before uploading.
- Remember platform-only consumers (`.mm`, `is_win`, `is_android`) are invisible
  to a Linux `gn check`.
