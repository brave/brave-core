# Vertical Tab Migration: Brave → Upstream

## Purpose

This document tracks the gap between **Brave's vertical tab strip** (fully
custom feature, shipped since ~2022) and **upstream Chromium's native vertical
tabs** feature (`tabs::kVerticalTabs` / `kVerticalTabsLaunch`, currently
Finch-gated, not yet on `chrome://flags`, not GA).

Today, Brave's implementation **disables itself** whenever upstream's feature
is enabled for a client (see
[`VerticalTabController::SupportsBraveVerticalTabs()`](../../../../../browser/ui/tabs/vertical_tab_controller.cc)),
because the two systems are structurally independent and would otherwise
conflict. As Brave starts adopting pieces of upstream's implementation (to
reduce patch surface and inherit upstream maintenance), every Brave-specific
behavior listed below needs to be **re-verified or re-implemented on top of
the upstream code**, or adoption will regress a shipped feature.

This is a planning/reference document for agents implementing the migration.
It is not a design spec for the final code — see the "Suggested order" section
for how to approach the work incrementally.

## Coexistence requirement (read first)

**We are keeping both Brave's implementation and upstream's implementation
side by side for a while.** This is not a rip-and-replace migration with a
hard cutover date. Concretely, this means:

- Brave's existing `VerticalTabController` /
  `BraveVerticalTabStripRegionView` / Tree Tab / all prefs listed in this doc
  **must keep working and stay shippable** throughout this effort. Do not
  delete, disable-by-default, or feature-flag-off any currently-shipped Brave
  behavior as a side effect of adopting upstream code.
- Any work that wires up upstream's `tabs::VerticalTabStripStateController` /
  `VerticalTabStripRegionView` should be **additive**: it must be possible to
  build and run with either backend active, selectable independently of
  upstream's own Finch/rollout state (Brave will need its own switch/flag for
  this — see the Open Questions section).
- The current auto-yield logic in `SupportsBraveVerticalTabs()` (turn Brave's
  UI off if upstream's feature happens to be on) is **not sufficient** as a
  long-term coexistence mechanism, because it can silently drop a feature a
  user depends on (left/right positioning, Tree Tabs, hide-completely, a
  non-default floating setting, the default shared-across-windows collapse
  state, a non-default scrollbar setting) the moment upstream's flag flips on
  for that client. Until upstream reaches full parity with the items in this
  doc, any backend-selection logic must **fall back to Brave's
  implementation** whenever the user relies on a capability upstream doesn't
  yet support, rather than silently downgrading them.
- Test coverage for both backends must be kept green in parallel — do not
  remove or skip Brave's existing unit/browser tests
  (`vertical_tab_controller_unittest.cc`, `vertical_tab_strip_browsertest.cc`,
  `tree_tabs_browsertest.cc`, etc.) as part of standing up upstream-backed
  equivalents.
- Treat every "Reimplementation plan" below as "build this as an option
  alongside the existing Brave path," not "replace the existing Brave path
  with this."
- This coexistence requirement is about **keeping Brave's implementation
  shippable**, not about growing `VerticalTabController`. See "Target
  end-state & implementation principle" in §0 — new work should land on
  `tabs::VerticalTabStripStateController` (via `chromium_src`), while
  Brave's existing `VerticalTabController`-based path is left alone/frozen
  until it can be deleted.

## Non-goals

This document does not cover Brave features that are unrelated to vertical
tabs even though they live in the same files (e.g. `kSharedPinnedTab`,
`kAlwaysHideTabCloseButton`, compact horizontal tabs). Only the items below
(§1-§7) plus the cross-cutting architecture concerns are in scope.

This document also does not propose a timeline or decision process for
eventually deprecating Brave's own implementation — that is out of scope
until upstream reaches feature parity and the coexistence period ends.

---

## 0. Cross-cutting architecture differences

These affect *every* item below and should be understood before starting any
of the individual migrations.

| Concern | Brave (today) | Upstream |
|---|---|---|
| Per-window "mode" controller | `VerticalTabController` — a **Brave-only** abstract type. `browser/ui/tabs/public/vertical_tab_controller.h`, impl in `browser/ui/tabs/vertical_tab_controller.cc`. Injected into `chrome/browser/ui/window_feature_controller/WindowFeatureController` via Brave patches (`patches/chrome-browser-ui-window_feature_controller-*.patch`) purely as a title-bar-suppression hook. | `tabs::VerticalTabStripStateController` — a real, per-`BrowserWindowInterface` `SessionServiceBaseObserver`-based state machine. `chrome/browser/ui/tabs/vertical_tab_strip_state_controller.h/.cc`. Owns collapse state, uncollapsed width, expand-on-hover pref, a `Delegate` interface for animation, ref-counted locks (`ScopedEnableStateLock`), toasts, and IPH triggers. |
| Main view | `BraveVerticalTabStripRegionView` (own class). `browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h/.cc`. Own `enum class State { kCollapsed, kFloating, kExpanded }`. | `VerticalTabStripRegionView` (`final`). `chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h/.cc`. Extends a new shared `BaseTabStripRegionView` (`chrome/browser/ui/views/frame/base_tab_strip_region_view.h`), parameterized by `TabStripOrientation{kHorizontal, kVertical}` (`chrome/browser/ui/views/tabs/shared/tab_strip_types.h`), as part of the `kTabStripUnification` effort. |
| Tab tree / collection model | Overrides `TabCollection` via `chromium_src/` (`brave_tab_strip_collection.cc`, `brave_tab_strip_collection_delegate.cc`) plus Brave's own `TreeTabNodeTabCollection` (see §4). | Same base `tabs::TabCollection` (`components/tabs/public/tab_collection.h`), but upstream now also mirrors it into a view-layer `TabCollectionNode` tree (`chrome/browser/ui/views/tabs/common/tab_collection_node.h`), kept in sync via `LINT.IfChange/ThenChange`. |
| Persistence | Plain profile prefs, global or per-window-flag (see `browser/ui/tabs/brave_tab_prefs.h`). | Prefs (`chrome/common/pref_names.h`) are only a **fallback**. The real per-window source of truth is `SessionService` window extra-data, keys `vertical_tab_strip_collapsed` / `vertical_tab_strip_uncollapsed_width` (`VerticalTabStripStateController::kCollapsedKey`/`kUncollapsedWidthKey`). |

**Implication:** this is not "flip a flag." And unlike an earlier draft of
this doc suggested, the end goal is **not** to keep `VerticalTabController`
as the permanent decision point — see "Target end-state & implementation
principle" immediately below, which supersedes that framing. Every
`chromium_src` override that currently branches on
`VerticalTabController::ShouldShowBraveVerticalTabs()` — `tab.cc`,
`tab_strip.cc`, `tab_group_views.cc`, `tab_group_style.cc`,
`horizontal_tab_style_views.cc`, `dragging/dragging_tabs_session.cc`,
`tab_strip_layout_helper.h/.cc`, `browser_view_layout_delegate*`,
`system_menu_model_delegate.cc`, `tab_search_bubble_host.cc` — will
eventually need to be re-pointed at `tabs::VerticalTabStripStateController`
(extended via `chromium_src`) instead of `VerticalTabController`, since
upstream's own `TabStripOrientation`-aware `Tab`/`TabStrip` code may already
do natively what Brave's overrides do manually. This is a patch-surface
reduction opportunity long-term, but during the coexistence period these call
sites keep working as-is (unmodified) until each one's underlying feature
gap is closed.

### Target end-state & implementation principle — read this before writing any code

**`VerticalTabController` is legacy and frozen.** The end state of this
migration is that `VerticalTabController` is deleted entirely and every
responsibility it currently has is absorbed into
`tabs::VerticalTabStripStateController` (upstream's real per-window
controller), extended via Brave's normal `chromium_src`-override mechanism.
The explicit goal of the whole migration is to **minimize Brave's deviation
from upstream** — reuse upstream's controller, upstream's view classes, and
upstream's persistence model wherever possible, rather than maintaining a
parallel Brave-owned controller/view stack indefinitely.

Concrete rules that follow from this, for every item in §1-§7:

- **Do not add new methods to `VerticalTabController`.** Do not add new call
  sites of `VerticalTabController::FromBrowser()` anywhere in the codebase
  (existing call sites are fine to leave as-is for now; see the re-auditing
  note above).
- Any new Brave-specific logic needed to support a §1-§7 item on the upstream
  backend (gating checks, new prefs, new behavior) should be implemented as
  a `chromium_src` extension **on `tabs::VerticalTabStripStateController`
  (or its `Delegate`/the upstream `VerticalTabStripRegionView`)**, following
  the same override pattern Brave already uses elsewhere in `chromium_src/`
  — not as an addition to `VerticalTabController`.
- The backend-selection mechanism (step 0 in "Suggested reimplementation
  order") is the most important place to get this right, since everything
  else builds on it: implement it as an extension of
  `tabs::VerticalTabStripStateController` from the start (e.g. a
  Brave-added method surfaced through the same class), not as a new
  `VerticalTabController` API.
- As each §2-§7 gap is closed on the upstream backend, actively look for
  opportunities to **shrink** `VerticalTabController` by deleting whichever
  of its methods/call sites are no longer needed, rather than only adding
  to it. The intent is a monotonically shrinking, never-growing class on the
  way to zero.

**Current self-disable logic** (must be replaced by real interop, not just
removed):

```cpp
// browser/ui/tabs/vertical_tab_controller.cc
bool VerticalTabController::SupportsBraveVerticalTabs() const {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          tabs::switches::kDisableVerticalTabsSwitch)) {
    return false;
  }
  if (tabs::IsVerticalTabsFeatureEnabled()) {
    // In case that Chromium's vertical tabs feature is enabled, we should not
    // show Brave's vertical tabs.
    return false;
  }
  return type_ == BrowserWindowInterface::TYPE_NORMAL;
}
```

---

## 1. Vertical-tab usage preference migration — **deprioritized: do this last**

> Per "Suggested reimplementation order" below, close the feature gaps in
> §2-§7 first. A full sync layer here has little payoff while users who rely
> on a missing feature are still gated onto Brave's backend anyway, and it
> would need reworking each time a gap closes. Only a minimal one-time
> bootstrap of `kVerticalTabsEnabled` is needed in the meantime.

| | Brave | Upstream |
|---|---|---|
| Enabled flag | `brave_tabs::kVerticalTabsEnabled` = `"brave.tabs.vertical_tabs_enabled"`, default `false`. Registered in `brave_tab_prefs.cc`. | `prefs::kVerticalTabsEnabled` = `"vertical_tabs.enabled"`, default `false`. Registered in `chrome/browser/ui/tabs/tab_strip_prefs.cc::RegisterProfilePrefs()`. |
| Collapsed state | `brave_tabs::kVerticalTabsCollapsed` (`"brave.tabs.vertical_tabs_collapsed"`, global) + `brave_tabs::kVerticalTabsExpandedStatePerWindow` (`"brave.tabs.vertical_tabs_expanded_state_per_window"`) to opt into per-window tracking. | `prefs::kVerticalTabsCollapsedState` (`"vertical_tabs.collapsed_state"`) — **fallback only**; real per-window value lives in `SessionService` extra data key `"vertical_tab_strip_collapsed"`. |
| Width | `brave_tabs::kVerticalTabsExpandedWidth` (`"brave.tabs.vertical_tabs_expanded_width"`, default `220`). | `prefs::kVerticalTabsUncollapsedWidth` (`"vertical_tabs.uncollapsed_width"`, default `kVerticalTabStripDefaultUncollapsedWidth = 240`) — fallback only; real value in session extra data key `"vertical_tab_strip_uncollapsed_width"`. |
| First-time / "ever enabled" tracking | None. | `prefs::kVerticalTabsEnabledFirstTime` (`"vertical_tabs.enabled_first_time"`) — metrics-only, drives `VerticalTabIphController`'s upsell heuristics. |

### Gap / plan

Because both backends must keep working (see Coexistence requirement), this
is **not** a one-way, destructive migration where Brave's prefs get copied
once and then ignored. A user must be able to flip between the Brave backend
and the upstream backend (directly, or because upstream's Finch rollout state
changes under them) without losing settings either direction.

- Keep Brave's prefs (`brave_tabs::kVerticalTabsEnabled` et al.) as the
  **canonical, user-facing source of truth** for now, since Brave's Settings
  UI reads/writes them and they must keep working regardless of which
  backend is active.
- When the upstream backend is the one actually active for a window, **sync**
  (not one-shot-migrate-and-forget) the relevant Brave pref values into
  upstream's prefs/session-extra-data on each change, so upstream's own code
  (which reads its own prefs/session data directly) observes the same state
  the user configured via Brave's UI. Treat this as a translation layer, not
  a migration — it needs to run continuously, not just once at startup.
- The two `enabled` prefs share the same type and default (`bool`, `false`),
  so keeping them in sync is simple. Follow the existing default-guarded
  pattern in `brave_tabs::MigrateBraveProfilePrefs()` (`brave_tab_prefs.cc`,
  used today for `kVerticalTabsShowScrollbar`) only for true one-time
  bootstrapping (e.g. the very first time the upstream backend is exercised
  for a profile) — not as the general sync mechanism.
- Collapsed/width state: upstream treats its own prefs as fallback and
  session-extra-data as truth. The sync layer must write **both** when the
  upstream backend is active, or a restored window will silently ignore
  synced pref values in favor of stale/absent session data.
- `kVerticalTabsExpandedStatePerWindow` is its own gap, not just a migration
  detail — see §6 below.

---

## 2. Left/right positioning — **absent upstream, must be reimplemented**

Upstream only supports RTL-locale mirroring (`base::i18n::IsRTL()` checks
inside `VerticalTabStripRegionView` for resize-drag direction, drop-arrow
direction, collapse-icon direction). There is **no pref, enum, or setting**
for a user-facing "put my tabs on the right" toggle — the strip is always
docked to the layout-leading edge.

### What Brave has today

- Pref: `brave_tabs::kVerticalTabsOnRight` (`"brave.tabs.vertical_tabs_on_right"`,
  default `false`).
- `VerticalTabController::IsVerticalTabOnRight()`
  (`vertical_tab_controller.cc:101`).
- `BraveVerticalTabStripRegionView` layout/resize logic branches on the pref
  for: resize-handle x-position, hot-corner detection rect side, resize-drag
  sign inversion (`vertical_tab_strip_region_view.cc`).
- Frame-layout bounds computation:
  `chromium_src/chrome/browser/ui/views/frame/layout/brave_browser_view_tabbed_layout_impl.cc`
  (`outer_left`/`outer_right`, `has_left_ui`/`has_right_ui`).
- Toolbar toggle-button placement:
  `browser/ui/views/toolbar/brave_toolbar_view.cc`
  (`UpdateVerticalTabTogglePlacement()`, handles RTL interplay).
- Sidebar auto-flip-to-avoid-overlap interaction:
  `sidebar::kSidebarAlignmentChangedTemporarily`
  (`components/sidebar/browser/pref_names.h`), tested in
  `browser/ui/sidebar/sidebar_browsertest.cc`.
- Settings UI: `browser/resources/settings/brave_appearance_page/tabs.html`
  (`settings-radio-group[pref-key="brave.tabs.vertical_tabs_on_right"]`).

### Reimplementation plan

1. Re-add `kVerticalTabsOnRight` pref (own namespace; no upstream collision).
2. Re-implement layout/resize/hot-corner side logic against the new upstream
   `VerticalTabStripRegionView` / `BaseTabStripRegionView`.
3. Re-implement frame outer-bounds logic for whichever browser-view layout
   class upstream ends up using post-`kTabStripUnification`.
4. Re-implement toolbar toggle-button placement.
5. Keep the sidebar-alignment auto-flip interaction; re-verify it still
   triggers correctly against the new view's resize/visibility signals.
6. Port the settings UI radio group; update `isRTL`/mirroring assumptions
   since upstream's own RTL handling may now interact with this toggle
   (e.g. RTL + "on right" = which physical side?).
7. **Coexistence gate:** until this lands on the upstream backend, backend
   selection must keep a user on Brave's implementation if
   `kVerticalTabsOnRight` is non-default (`true`) for their profile, even if
   upstream's vertical tabs feature is otherwise active for them.

---

## 3. Tree Tab — **absent upstream, largest reimplementation effort**

Upstream's `TabCollection` tree only allows `GROUP`/`SPLIT` as child types of
`UNPINNED`, and `TabGroupTabCollection` explicitly declares
`supported_child_collections = {TabCollection::Type::SPLIT}` — **groups
cannot nest groups**. There is no collapsible, arbitrary-depth hierarchy
concept upstream, i.e. no analogue of Tree Tabs at all.

### What Brave has today

See `docs/chrome/browser/ui/tabs/tree_tabs_architecture.md` and
`tree_tabs_session_restore.md` for full detail. Summary of key types:

- `tabs::TreeTabNode` (`components/tabs/public/tree_tab_node.h`,
  `components/tabs/impl/tree_tab_node.cc`) — metadata: id, level, height,
  collapsed.
- `tabs::TreeTabNodeTabCollection` (extends `TabCollection`, new
  `Type::TREE_TAB_NODE`) — the actual nesting container.
  `components/tabs/public/tree_tab_node_tab_collection.h`,
  `components/tabs/impl/tree_tab_node_tab_collection.cc`.
- `TreeTabModel` (`browser/ui/tabs/tree_tab_model.h/.cc`) — registry/lookup,
  collapsed-ancestor caching.
- `BraveTreeTabStripCollectionDelegate`
  (`browser/ui/tabs/brave_tree_tab_strip_collection_delegate.h/.cc`) —
  delegate hook for tab-manipulation-aware tree behavior (move into/out of
  group, split interplay).
- `BraveTabStripModel` (`browser/ui/tabs/brave_tab_strip_model.h/.cc`) — owns
  `tree_model()`, gates on `tabs::kBraveTreeTab`.
- `TreeTabSessionManager` (`browser/ui/tabs/tree_tab_session_manager.h/.cc`)
  — persists hierarchy via generic session `AddTabExtraData` keys
  (`brave_tree_node_id`, `brave_tree_parent_node_id`,
  `brave_tree_node_collapsed`; see `browser/sessions/brave_session_keys.h`).
- Rendering: `browser/ui/views/tabs/brave_tab.cc`, `brave_tab_strip.cc`,
  `brave_tab_container.cc`, `brave_browser_tab_strip_controller.cc` — all
  gated on `base::FeatureList::IsEnabled(tabs::kBraveTreeTab)`.
- Pref: `brave_tabs::kTreeTabsEnabled` (`"brave.tabs.tree_tabs_enabled"`),
  feature-gated by `tabs::kBraveTreeTab`.

### Reimplementation plan

1. Add a new `TabCollection::Type::TREE_TAB_NODE` (and mirrored
   `TabCollectionNode::Type`) on top of the **new, unified**
   `BaseTabStripRegionView`/`TabCollectionNode` infra, following the same
   `LINT.IfChange/ThenChange` discipline upstream uses for its own
   group/split types, rather than layering purely inside `chromium_src`.
2. Re-home rendering/indentation logic (currently in `brave_tab.cc` /
   `brave_tab_strip.cc` / `brave_tab_container.cc`) onto upstream's new
   orientation-aware painting code (`tab_view_vertical_layout.h`,
   `vertical_tab_style_views.h`) instead of Brave's previous ad hoc paint
   overrides.
3. Re-verify `BraveTreeTabStripCollectionDelegate`'s interplay logic
   (group/split wrapping, `MoveTabsIntoGroup`/`MoveTabsOutOfGroup`) against
   any upstream changes to `TabGroupTabCollection`/`SplitTabCollection`
   manipulation APIs.
4. `TreeTabSessionManager`'s persistence format is storage-agnostic
   (generic extra-data keys) and should port with minimal change — but
   confirm no key collisions with upstream's own new session extra-data
   keys (`vertical_tab_strip_collapsed`, `vertical_tab_strip_uncollapsed_width`).
5. This is independent of horizontal-vs-vertical mode in principle (Tree
   Tabs could theoretically apply to horizontal tabs too), but Brave
   currently only surfaces it under vertical tabs UI — confirm whether that
   scoping is intentional/required post-migration.
6. **Coexistence gate:** since upstream has no Tree Tab analogue at all,
   backend selection must keep a user on Brave's implementation whenever
   `brave_tabs::kTreeTabsEnabled` is `true` for their profile, for as long as
   this item remains unimplemented on the upstream backend.

---

## 4. Floating mode — **closest upstream analogue exists; reconcile, don't reinvent**

Upstream's `kVerticalTabsExpandOnHover` (Finch-gated, off by default via
`kVerticalTabsExpandOnHoverDefaultEnabled` param) is a mature reimplementation
of the same idea as Brave's floating mode.

### What Brave has today

- Pref: `brave_tabs::kVerticalTabsFloatingEnabled`
  (`"brave.tabs.vertical_tabs_floating_enabled"`, default `true`).
- `VerticalTabController::IsFloatingVerticalTabsEnabled()`
  (`vertical_tab_controller.cc:80`) — forces `true` when "hide completely
  when collapsed" is on, or when the toggle button is hidden (no other way
  to expand); otherwise reads the pref.
- `BraveVerticalTabStripRegionView`: `enum class State { kCollapsed,
  kFloating, kExpanded }`; hover/mouse-exit timers
  (`ScheduleFloatingModeTimer()`/`ScheduleCollapseTimer()`), CLI overrides
  `tabs::switches::kVerticalTabExpandDelaySwitch` /
  `kVerticalTabCollapseDelaySwitch`; hot-corner detection; forced floating
  during browser fullscreen / Focus Mode
  (`IsFloatingEnabledForBrowserMode()`, `UpdateFloatingStateForBrowserMode()`,
  `floating_restore_state_`).

### What upstream has

- Feature `tabs::kVerticalTabsExpandOnHover` + pref
  `prefs::kVerticalTabsExpandOnHoverEnabled`
  (`"vertical_tabs.expand_on_hover"`).
- Two expansion strategies: fixed-delay
  (`kVerticalTabsExpandOnHoverDelay`/`...ClickDelay` params) or mouse-velocity
  heuristic (`...UseVelocityHeuristic` + 6 tuning params), implemented in
  `VerticalTabStripRegionView`.
- `VerticalTabStripExpandOnHoverLock` / `ExpandOnHoverLockType`
  (`chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_expand_on_hover_lock.h/.cc`)
  — a ref-counted lock system so other UI (omnibox popup open, link drag) can
  pin the strip open/closed/force-collapsed.
- IPH/tutorial: `VerticalTabStripStateController::MaybeShowExpandOnHoverIPH()`,
  `feature_engagement::kIPHVerticalTabsExpandOnHoverFeature`.

### Reimplementation / reconciliation plan

1. Force-enable `tabs::kVerticalTabsExpandOnHover` and set its params to
   match Brave's current UX defaults (Brave defaults floating **on**;
   upstream's Finch default is currently effectively off).
2. Migrate `kVerticalTabsFloatingEnabled` → `prefs::kVerticalTabsExpandOnHoverEnabled`
   (see §1 migration approach).
3. Re-implement Brave's forced-floating cases (fullscreen, Focus Mode, hide-
   completely-when-collapsed, no-toggle-button) as **lock acquisitions**
   against upstream's `VerticalTabStripExpandOnHoverLock` /
   `ScopedEnableStateLock`, rather than re-deriving a parallel state machine.
   This lock system looks purpose-built for exactly these cases.
4. Decide fate of the CLI debug switches
   (`kVerticalTabExpandDelaySwitch`/`kVerticalTabCollapseDelaySwitch`) — likely
   replaced by upstream's Finch params (`kVerticalTabsExpandOnHoverDelay`,
   etc.), possibly wired as param overrides for local dev/QA instead of a
   command-line switch.
5. Re-verify hot-corner detection behavior matches (upstream's velocity
   heuristic vs. Brave's simple hover-rect timer) — this is a UX-visible
   behavior change to validate with design/QA.
6. **Coexistence gate:** floating mode is the one item with a close upstream
   analogue, but until steps 1-5 are validated to match Brave's UX, keep
   Brave's implementation as the default and treat upstream's
   expand-on-hover as opt-in/experimental for this feature specifically.

---

## 5. "Hide completely when collapsed" — **absent upstream, must be reimplemented**

Upstream's collapsed state is hardcoded to a 56px icon rail
(`VerticalTabStripRegionView::kCollapsedWidth = 56`) — it is never 0px, and
there is no pref or mode for fully hiding the strip.

### What Brave has today

- Feature: `tabs::kBraveVerticalTabHideCompletely`
  (`chromium_src/chrome/browser/ui/tabs/features.h/.cc`,
  `FEATURE_ENABLED_BY_DEFAULT`). Flag: `brave-vertical-tab-hide-completely`
  in `browser/about_flags.cc`.
- Pref: `brave_tabs::kVerticalTabsHideCompletelyWhenCollapsed`
  (`"brave.tabs.vertical_tabs_hide_completely_when_collapsed"`) — only
  registered when the feature is enabled.
- `VerticalTabController::ShouldHideVerticalTabsCompletelyWhenCollapsed()`
  (`vertical_tab_controller.cc:105`) = feature-enabled AND pref-true.
- `BraveVerticalTabStripRegionView::GetPreferredWidthForState()` — collapsed
  width becomes `0` in this mode; `GetMinimumSize()` special-cased to return
  zero size; `OnHideComopletelyWhenCollapsedPrefChanged()` (typo preserved in
  source) toggles visibility and re-invokes floating-mode logic.
- Interaction: when on, floating mode is always forced on (see §4) since
  there's otherwise no way to reveal a 0-width strip.

### Reimplementation plan

1. Re-add feature flag + pref (own namespace, no upstream collision).
2. Reimplement the width-override in upstream's
   `VerticalTabStripRegionView` width-calculation path (the equivalent of
   today's `GetPreferredWidthForState()`/`GetMinimumSize()` special-casing)
   and its `Delegate`-driven collapse animation
   (`tabs::VerticalTabStripStateController::Delegate`).
3. Reimplement the "always force floating/expand-on-hover when
   hide-completely is on" interaction via the same lock mechanism used in
   §4 (`VerticalTabStripExpandOnHoverLock`) rather than a bespoke check.
4. Port settings UI (`isHideVerticalTabCompletelyFlagEnabled` gated
   checkbox in `tabs.html`/`tabs.ts`).
5. **Coexistence gate:** backend selection must keep a user on Brave's
   implementation whenever
   `brave_tabs::kVerticalTabsHideCompletelyWhenCollapsed` is `true` for their
   profile, for as long as this item remains unimplemented on the upstream
   backend.

---

## 6. Collapsed-state scope: shared-across-windows vs. per-window — **Brave's *default* differs from upstream's unconditional per-window model**

Unlike the other gaps, this one runs in the opposite direction: upstream is
**always** per-window (and properly persisted), while Brave's shipped
*default* is a single collapsed/width state **shared across every open
window**, with per-window as an opt-in. Naively adopting upstream's backend
would silently change default behavior for most users (whose "per window"
setting is off), not just an edge case.

### What Brave has today

- Pref: `brave_tabs::kVerticalTabsExpandedStatePerWindow`
  (`"brave.tabs.vertical_tabs_expanded_state_per_window"`), default `false`
  (i.e. shared/global state is the default).
- `chrome::ToggleVerticalTabStripExpanded()` (`browser/ui/browser_commands.cc`)
  — when the pref is `false`, toggles the single global
  `brave_tabs::kVerticalTabsCollapsed` pref (which every window observes and
  reacts to). When `true`, it instead calls
  `BraveVerticalTabStripRegionView::ToggleState()` on only the current
  window's view.
- **Important implementation nuance:** even in "per-window" mode, there is
  **no real per-window persistence** — no window-keyed dictionary pref, no
  session-extra-data. `BraveVerticalTabStripRegionView::OnCollapsedPrefChanged()`
  and `OnExpandedWidthPrefChanged()` (`vertical_tab_strip_region_view.cc`)
  simply early-return (ignore live updates to the shared pref) when the
  per-window pref is `true`, decoupling each window's in-memory `state_` from
  other windows' subsequent changes — but every window still *seeds* its
  initial state from the shared `kVerticalTabsCollapsed`/`kVerticalTabsExpandedWidth`
  pref values at construction time, and nothing survives restart per-window.
  So Brave's "per-window" mode today is strictly weaker than upstream's
  always-per-window + `SessionService`-persisted model.
- Settings UI: `tabs.html` — `settings-checkbox[pref-key="brave.tabs.vertical_tabs_expanded_state_per_window"]`,
  label **"Auto expand vertical tabs independently per window"**.

### What upstream has

- No concept of "shared/global" at all. `tabs::VerticalTabStripState` is
  documented in-code as "Per-window state for the vertical tab strip"
  (`vertical_tab_strip_state.h`), owned one-per-window by
  `tabs::VerticalTabStripStateController`, and durably persisted per-window
  via `SessionService` extra data (`vertical_tab_strip_collapsed` /
  `vertical_tab_strip_uncollapsed_width` keys,
  `VerticalTabStripStateController::UpdateSessionService()`).
- The upstream fallback prefs (`prefs::kVerticalTabsCollapsedState` /
  `kVerticalTabsUncollapsedWidth`) only seed a **brand-new, non-session-restored**
  window (e.g. via "New Window") with the most-recently-seen state — they are
  never used to keep multiple *currently open* windows in sync with each
  other. There is no toggle to force a single shared state across open
  windows.

### Reimplementation plan

1. Decide whether Brave keeps offering "one collapsed/width state shared
   across all open windows" (matching today's default) on top of the
   upstream backend. Given it's the current default for essentially all
   users, this should be treated as a required item, not optional polish.
2. If kept, implement it as a Brave-specific extension on top of upstream's
   per-window model: on any window's collapse/width change, propagate the
   same value into every other open window's live
   `tabs::VerticalTabStripStateController` state *and* its `SessionService`
   extra data, rather than relying on upstream's single-window persistence.
3. Re-point the existing Settings toggle at this new mechanism. Keep the
   **default** matching today's shipped default (shared/global) so migrated
   users see no behavior change unless they've already opted into per-window.
4. Note the toggle's effective meaning inverts under upstream: today it's
   framed as "opt in to per-window"; on top of upstream (which is
   unconditionally per-window) it effectively becomes "opt in to
   forced-sync-across-windows" instead. Revisit the string/label for clarity.
5. Users who already have `kVerticalTabsExpandedStatePerWindow = true` are,
   if anything, a good early-adopter candidate for the upstream backend,
   since upstream's per-window persistence is a strict improvement over
   Brave's current in-memory-only per-window shim.
6. **Coexistence gate:** keep users with `kVerticalTabsExpandedStatePerWindow`
   at its default (`false`, i.e. relying on shared/global state) on Brave's
   backend until step 2's cross-window sync is implemented on the upstream
   backend — this affects the default experience for most users, not an
   edge case, so treat it with the same priority as the other "must
   implement before wider upstream rollout" gaps.

---

## 7. Scrollbar visibility toggle — **absent upstream, must be reimplemented**

Upstream's vertical tab strip always scrolls (never shrinks tab size) when
tabs overflow, and always shows a persistent, custom-drawn scrollbar while
doing so — there is no pref or feature to hide it.

### What Brave has today

- Feature: `tabs::kBraveVerticalTabScrollBar`
  (`chromium_src/chrome/browser/ui/tabs/features.cc`,
  `FEATURE_DISABLED_BY_DEFAULT` — this is an experimental/opt-in flag, not a
  default-on behavior for regular users today). Flag:
  `brave-vertical-tab-scroll-bar` in `browser/about_flags.cc`.
- Pref: `brave_tabs::kVerticalTabsShowScrollbar`
  (`"brave.tabs.vertical_tabs_show_scrollbar"`), default `false`; migrated to
  `true` (default-guarded, one-time) when `kBraveVerticalTabScrollBar` is
  enabled — see `brave_tabs::MigrateBraveProfilePrefs()`
  (`brave_tab_prefs.cc`).
- Consumer: `browser/ui/views/tabs/brave_tab_container.cc` —
  `BraveTabContainer::GetScrollBarMode()` maps the pref (plus floating-mode/
  collapsed-state checks) to a `views::ScrollView::ScrollBarMode`
  (`kEnabled` / `kHiddenButEnabled` / `kDisabled`). It's a custom-drawn
  `views::ScrollBar` (via `views::PlatformStyle::CreateScrollBar`), not the
  OS-native scrollbar. When the pref is off, the scrollbar track/thumb is
  hidden but scrolling still works (`kHiddenButEnabled`); it's also
  force-hidden while floating vertical tabs are collapsed (grabbing it would
  just re-expand the strip).
- Settings UI: `tabs.html` —
  `settings-checkbox[pref-key="brave.tabs.vertical_tabs_show_scrollbar"]`,
  label **"Show scrollbar"**.

### What upstream has

- `chrome::VerticalTabStripScrollBar`
  (`chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_scroll_bar.h/.cc`),
  a `tabs::RoundedScrollBar` subclass, instantiated unconditionally for the
  vertical orientation in `chrome/browser/ui/views/tabs/common/tab_strip_view.cc`
  (`SetScrollViewProperties()`), always in `kEnabled`/visible mode (only
  temporarily flipped to `kHiddenButEnabled` during resize animations to
  avoid jank — an animation-smoothing detail, not a user setting).
  `VerticalTabStripScrollBar::ShouldHaveRightMargin()` only adjusts margin
  based on collapse state; there is no visibility-toggling pref/feature
  anywhere under `chrome/browser/ui/views/tabs/vertical/` or
  `chrome/browser/ui/tabs/features.h/.cc`.

### Reimplementation plan

1. Re-add feature flag + pref (own namespace, no upstream collision).
2. Gate the vertical `ScrollView`'s `ScrollBarMode` (`kEnabled` vs.
   `kHiddenButEnabled`) on the pref at the `TabStripView`/scroll-view level,
   mirroring `BraveTabContainer::GetScrollBarMode()`'s logic, including the
   floating-mode-collapsed suppression (tie this to whatever lock mechanism
   §4 ends up using).
3. Port the Settings UI checkbox.
4. Confirm no conflict with upstream's own `ShouldHaveRightMargin()`
   margin-adjustment behavior.
5. **Coexistence gate:** since `kBraveVerticalTabScrollBar` is
   disabled-by-default today, this is low-priority/low-risk relative to the
   other gaps — most users have no visible-scrollbar expectation to
   preserve. Still gate on `brave_tabs::kVerticalTabsShowScrollbar` being
   `true` (whether via the feature-driven migration or explicit user choice)
   until implemented, so the minority who do have it on don't regress.

---

## Suggested reimplementation order

**Priority: close upstream's feature gaps first (§2-§7). Do the duplicated-
preference migration/sync work (§1) last.** Rationale: while any of §2-§7
remain unimplemented upstream, the backend-selection gate (step 0 below)
keeps every user who relies on that capability on Brave's implementation
anyway — so a full bidirectional, continuously-synced pref layer between
Brave's prefs and upstream's prefs/session-data has very little real payoff
yet, and would have to be reworked every time a new gap closes and a pref's
meaning/gating changes. It's cheaper to finalize the pref sync once *after*
the upstream backend actually supports the full feature set and the pref
surface has stabilized, rather than maintaining a moving-target sync layer
throughout.

0. **Backend-selection mechanism** — before any of §2-§7, something needs an
   explicit notion of "which backend is active for this window" (Brave vs.
   upstream) that is: (a) independently switchable from upstream's own
   Finch/rollout state via a Brave-owned flag/switch, and (b) automatically
   falls back to Brave's backend when the user depends on a capability
   upstream doesn't yet support (per the per-item "Coexistence gate" notes
   below). **Per "Target end-state & implementation principle" above, build
   this as a `chromium_src` extension of `tabs::VerticalTabStripStateController`,
   not as a new method on `VerticalTabController`.** It only needs to read
   Brave's *existing* prefs directly (no new sync layer) to make the
   fallback decision — full §1 sync is not a prerequisite. Everything else
   in this doc assumes this mechanism exists.
1. **Floating mode → upstream expand-on-hover** (§4) — closest fit,
   validates the lock-based approach that §2/§3/§5/§7 also depend on. Keep
   Brave's implementation as the default while this is validated.
2. **Hide-completely-when-collapsed** (§5) — small, layers directly on top
   of §4's width/lock plumbing.
3. **Collapsed-state shared-across-windows sync** (§6) — moderate effort,
   but prioritize it above the remaining items: it's Brave's shipped
   *default* behavior (not an opt-in edge case), so leaving it unported
   longest would mean the majority of users stay gated onto Brave's backend
   the longest.
4. **Scrollbar visibility toggle** (§7) — small; low priority since the
   underlying Brave feature is disabled-by-default today, so few users are
   actually gated on it.
5. **Left/right positioning** (§2) — moderate effort, touches layout/frame
   code but introduces no new data model.
6. **Tree Tab** (§3) — largest effort; requires extending the new
   `TabCollection`/`TabCollectionNode` type system and re-hosting rendering
   on the unified orientation-aware tab painting code. Should be tackled
   last among the feature-gap items, once the underlying view/collection
   replatform from items 1-5 has landed and stabilized.
7. **Preference/state sync scaffolding** (§1) — deliberately last. Once
   §2-§6 above have closed the feature gaps and the upstream backend is a
   viable full replacement for a given user, build the continuous sync (or
   decide on a different reconciliation strategy) between Brave's prefs and
   upstream's prefs/session-data so users can move between backends (or be
   migrated permanently) without losing settings. Until this step, a minimal
   one-time bootstrap (copy Brave's `kVerticalTabsEnabled` value into
   upstream's pref, default-guarded, same pattern as
   `MigrateBraveProfilePrefs()`) is sufficient for early dogfooding/testing
   of the upstream backend — it does not need to be the full sync described
   in §1 until this step is reached.

Note: this order describes when each capability becomes *available* on the
upstream backend, not when Brave's own implementation gets removed. Per the
Coexistence requirement, Brave's implementation stays the default/fallback
for any user relying on a not-yet-ported capability until a separate,
future decision is made to end the coexistence period.

## Open questions / risks

- Does upstream's `kTabStripUnification` (separate WIP flag, shares
  horizontal/vertical infra) need to land first, or can vertical-only
  migration proceed independently? This affects how much rework §2-§7 need.
- Upstream's `kVerticalTabs`/`kVerticalTabsLaunch` have **no `chrome://flags`
  entry** — they're Finch/server-config-only ("enforced via plaster
  rewrite"). Brave will likely need its own flag/switch to force-enable them
  in Brave builds independent of Google's rollout state, similar to how
  `kBraveVerticalTabHideCompletely` etc. are exposed today.
- Upstream ships a full onboarding tutorial and IPH promos
  (`kVerticalTabsTutorialId`, `kIPHVerticalTabstripTutorialFeature`,
  `kIPHVerticalTabsExpandOnHoverFeature`, `VerticalTabIphController`) that
  Brave currently disables (see
  `components/feature_engagement/public/feature_constants.cc` patch). Decide
  whether to keep disabling these or adopt them once on upstream's
  implementation.
- Confirm whether `CommandShowVerticalTabs` in
  `TabStripModel::ContextMenuCommand` (vestigial upstream, hits
  `NOTREACHED()`) vs. the real `CommandToggleVertical` /
  `chrome::ToggleVerticalTabs()` / `kActionToggleVerticalTabs` action path
  matters for any Brave context-menu integration.
- **How long is "for a while"?** No end date is defined for the coexistence
  period. Running two backends means double the test matrix (browser tests,
  interactive UI tests, manual QA) and double the bug surface for as long as
  it lasts — worth periodically revisiting via telemetry (e.g. what fraction
  of users would lose no functionality by switching to the upstream backend)
  rather than leaving it open-ended by default.
- Need a concrete mechanism for the per-item "Coexistence gate" checks (does
  this user rely on a Brave-only capability?) to live in one place —
  per "Target end-state & implementation principle," this should be a
  `chromium_src`-added method on `tabs::VerticalTabStripStateController`
  (e.g. `RequiresBraveVerticalTabsBackend()`), **not** a new
  `VerticalTabController` method, queried before any per-window backend
  decision, rather than duplicated ad hoc checks across §2/§3/§5/§6/§7.
- Decide how a user-visible backend switch (if one is ever exposed, vs. it
  being fully automatic/internal) should be surfaced, if at all, during the
  coexistence period — e.g. a `chrome://flags` entry for QA/dogfooding vs.
  no user-facing control at all until parity is reached.
