# Tree Tabs Session Restore

## Overview

This document describes the design for persisting Brave's `TreeTabNode` hierarchy
across browser sessions. Two distinct scenarios require handling:

1. **Browser restart** — closing all windows and reopening; uses `SessionService`.
2. **Close-tab-and-restore** — Ctrl+Z / History > Recently Closed; uses
   `TabRestoreService`.

See [tree_tabs_architecture.md](tree_tabs_architecture.md) for background on the
`TreeTabNode` and `TreeTabNodeTabCollection` model.

---

## Background: How Tab Groups and Split Tabs Are Persisted

The session service already persists two kinds of tab structural metadata — tab
groups and split tabs — using a layered approach:

1. **`SessionTab::group`** / **`SessionTab::split_id`** — per-tab membership fields
   in `session_types.h`.
2. **`SessionTabGroup`** / **`SessionSplitTab`** structs in `SessionWindow` — hold
   per-group/split visual metadata.
3. **Session commands** `kCommandSetTabGroup`, `kCommandSetTabGroupMetadata2`,
   `kCommandSetSplitTab`, `kCommandSetSplitTabData` — emitted by
   `SessionService::BuildCommandsForTab` for full rebuilds and incrementally on
   live changes.
4. **`RestoreSessionFromCommands`** — replays commands and populates
   `SessionTab::group` and `SessionWindow::tab_groups` in memory.
5. **`RestoreTabsToBrowser` → `RestoreTabGroupMetadata`** — rebuilds group
   structures in the live tab strip, remapping old session IDs to fresh ones.

Tree tab restoration follows the same five-layer pattern, with one key difference:
since `tree_tab::TreeTabNodeId` is a Brave-specific type, we use the existing
**`kCommandAddTabExtraData`** mechanism (id=33) rather than adding new fields to
Chromium's `session_types.h`.

---

## Data Stored Per Tab

Three keys are written into `SessionTab::extra_data` (and `tab_restore::Tab::extra_data`)
for each tab in tree mode:

| Key | Value | Notes |
|-----|-------|-------|
| `brave_tree_node_id` | `TreeTabNodeId` serialized as base::Token hex | The node this tab belongs to |
| `brave_tree_parent_node_id` | Parent's token hex, or `""` for a root node | Encodes the parent-child edge |
| `brave_tree_node_collapsed` | `"1"` or `"0"` | Written only on the primary tab of each node (the tab that is the direct `current_value` of the `TreeTabNodeTabCollection`, i.e. `current_value_type_ == kTab`) |

For tree nodes whose `current_value` is a `TabGroupTabCollection` or
`SplitTabCollection`, `brave_tree_node_id` and `brave_tree_parent_node_id` are
written on the first tab in the group/split, and `brave_tree_node_collapsed` is
omitted (group/split nodes cannot be individually collapsed today).

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                  SAVE PATH — Browser Restart                        │
│                                                                     │
│  SessionService::WindowOpened (BRAVE_SESSION_SERVICE_WINDOW_OPENED) │
│    → creates BraveSessionTreeTabHelper per browser window           │
│              │                                                      │
│              ▼                                                      │
│  BraveSessionTreeTabHelper (TabStripModelObserver)                  │
│    OnTreeTabChanged(kNodeCreated)  ──────────────────────────────┐  │
│    OnTreeTabChanged(kNodeCollapsedStateChanged)                  │  │
│    → SessionService::SetTreeTabNodeData                          │  │
│    → ScheduleCommand(CreateAddTabExtraDataCommand(...))  ────────┘  │
│                                                                  │  │
│  SessionService::WindowClosing (BRAVE_SESSION_SERVICE_WINDOW_CLOSING)│
│    → ScheduleResetCommands() while browser still in browser list    │
│    → BuildCommandsFromBrowsers → BuildCommandsForTab             │  │
│    → BRAVE_SESSION_SERVICE_BUILD_COMMANDS_FOR_TAB                │  │
│      (AppendRebuildCommand for all three keys)  ─────────────────┘  │
│              │                                                      │
│  SessionService::~SessionService → Save()                           │
│    → flushes rebuild buffer + pending commands to disk              │
│              │                                                      │
│              ▼                                                      │
│          Session file on disk                                       │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                  RESTORE PATH — Browser Restart                     │
│                                                                     │
│  RestoreSessionFromCommands                                         │
│    kCommandAddTabExtraData → SessionTab::extra_data populated       │
│              │                                                      │
│              ▼                                                      │
│  BraveTabStripModel constructor                                     │
│    OnTreeTabRelatedPrefChanged() → BuildTreeTabs()                  │
│    → BraveTreeTabStripCollectionDelegate set on collection          │
│              │                                                      │
│              ▼                                                      │
│  RestoreTabsToBrowser  (tabs added flat, each in its own            │
│    TreeTabNodeTabCollection with a fresh generated ID)              │
│              │                                                      │
│              ▼                                                      │
│  RestoreTabGroupMetadata  (existing, unchanged)                     │
│              │                                                      │
│              ▼                                                      │
│  BRAVE_RESTORE_TREE_TAB_NODES                                       │
│  → BraveRestoreTreeTabNodeMetadata                                  │
│    1. Build old_id → TreeTabNodeTabCollection* map                  │
│       (using initial_tab_count + tab_visual_index as browser index) │
│    2. Reparent collections to recreate tree hierarchy               │
│    3. Apply collapsed state via BraveTabStripModel::SetTreeTabNodeCollapsed │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│               SAVE PATH — Close Tab and Restore (Ctrl+Z)            │
│                                                                     │
│  BrowserLiveTabContext::GetExtraDataForTab                          │
│  (BRAVE_GET_EXTRA_DATA_FOR_TAB chromium_src override)               │
│    → BravePopulateTreeTabExtraData                                  │
│      writes brave_tree_node_id + brave_tree_parent_node_id          │
│              │                                                      │
│              ▼                                                      │
│          tab_restore::Tab::extra_data (in-memory)                   │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│             RESTORE PATH — Close Tab and Restore (Ctrl+Z)           │
│                                                                     │
│  BrowserLiveTabContext::AddRestoredTab                              │
│  (BRAVE_ADD_RESTORED_TAB chromium_src override)                     │
│    → BraveRestoreTabTreeHierarchy                                   │
│      scans live strip for parent by its current node ID             │
│      → MaybeRemoveCollection + AddCollection to reparent            │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Save Side — Browser Restart

### Incremental Updates (`BraveSessionTreeTabHelper`)

`BraveSessionTreeTabHelper` is a `TabStripModelObserver` that observes tree
structure changes for a single browser window and writes them to the session
file via `SessionService::SetTreeTabNodeData` (which calls `ScheduleCommand`):

- **`OnTreeTabChanged(kNodeCreated)`** — schedules the three `extra_data`
  commands for the new node. This covers the common case where a child tab is
  opened with an opener (e.g., Ctrl+T from a parent tab).
- **`OnTreeTabChanged(kNodeCollapsedStateChanged)`** — schedules an updated
  `brave_tree_node_collapsed` command for the node.

One `BraveSessionTreeTabHelper` instance is created per browser window in
`SessionService::WindowOpened` via the `BRAVE_SESSION_SERVICE_WINDOW_OPENED`
macro. Instances are stored in a file-scope static map keyed by `SessionService*`
and removed in `SessionService::~SessionService` via `BRAVE_SESSION_SERVICE_DESTRUCTOR`.

**Note**: `BraveSessionTreeTabHelper::OnTreeTabChanged` does NOT handle explicit
reparenting (drag-and-drop of tabs between parents). Reparenting is captured
instead by the `WindowClosing` rebuild described below.

### Full Rebuild on Window Close (`BRAVE_SESSION_SERVICE_WINDOW_CLOSING`)

`SessionService::WindowClosing` is called while the browser is still alive in
the browser list. The `BRAVE_SESSION_SERVICE_WINDOW_CLOSING` macro immediately
calls `ScheduleResetCommands()`, which:

1. Calls `BuildCommandsFromBrowsers()` synchronously — iterates all open
   browsers including the closing one.
2. For each tab, `BRAVE_SESSION_SERVICE_BUILD_COMMANDS_FOR_TAB` walks up to
   the tab's `TreeTabNodeTabCollection` and emits `AppendRebuildCommand` for all
   three tree keys, capturing the **current** tree state (including any
   reparenting that happened since the last periodic rebuild).
3. Schedules a save timer; the `SessionService::~SessionService()` destructor
   calls `command_storage_manager()->Save()` which flushes the rebuild buffer
   to disk.

This ensures that even if the user closes the browser immediately after
rearranging tree structure (before the periodic rebuild timer fires), the
correct hierarchy is captured.

### Why Both Mechanisms Are Needed

| Scenario | Captured by |
|----------|-------------|
| New child tab opened (via opener) | `BraveSessionTreeTabHelper::OnTreeTabChanged(kNodeCreated)` |
| Collapsed state toggled | `BraveSessionTreeTabHelper::OnTreeTabChanged(kNodeCollapsedStateChanged)` |
| Explicit reparenting (drag-and-drop) | `BRAVE_SESSION_SERVICE_WINDOW_CLOSING` rebuild |
| All of the above at restart | Full rebuild flushed by `~SessionService` destructor |

---

## Restore Side — Browser Restart

### Timing

`BraveTabStripModel` is constructed before any tabs are added to the browser.
Its constructor calls `OnTreeTabRelatedPrefChanged()`, which calls
`BuildTreeTabs()` if both `kTreeTabsEnabled` and `kVerticalTabsEnabled` prefs
are true. This sets up `tree_model()` and the `BraveTreeTabStripCollectionDelegate`
**before** `RestoreTabsToBrowser` runs.

After `RestoreTabsToBrowser`, all restored tabs are flat root-level
`TreeTabNodeTabCollection` nodes (no parent-child relationships yet). Each has a
**freshly generated** `TreeTabNodeId` that is unrelated to the IDs in the session
file. `BRAVE_RESTORE_TREE_TAB_NODES` bridges this gap.

### `BraveRestoreTreeTabNodeMetadata`

Called from the `chromium_src` override of `session_restore.cc` (macro
`BRAVE_RESTORE_TREE_TAB_NODES`) after `RestoreTabGroupMetadata`.

#### Step 1 — Build the old-ID-to-collection map

```
for each session_tab in window->tabs:
    old_id = session_tab->extra_data["brave_tree_node_id"]
    browser_index = initial_tab_count + session_tab->tab_visual_index
    wc  = browser->tab_strip_model()->GetWebContentsAt(browser_index)
    tab = browser->tab_strip_model()->GetTabForWebContents(wc)
    collection = walk up from tab->GetParentCollection() until TREE_NODE
    old_id_to_coll[old_id] = collection
```

`tab_visual_index` equals the sequential DFS loop counter `index` from
`BuildCommandsForBrowser`, so `initial_tab_count + tab_visual_index` directly
addresses the correct position in the post-restore browser strip.

#### Step 2 — Reparent collections

Iterate tabs in `tab_visual_index` order (which is DFS order for the saved tree).
For each tab whose `brave_tree_parent_node_id` is non-empty:

```
parent_collection = old_id_to_coll[parent_old_id]
child_collection  = old_id_to_coll[old_id]
current_parent->MaybeRemoveCollection(child_collection)
parent_collection->AddCollection(child_collection, parent_collection->ChildCount())
```

Processing in DFS order guarantees parent collections exist in the map before
any of their children are reparented.

#### Step 3 — Restore collapsed state

After reparenting, for each tab carrying `brave_tree_node_collapsed = "1"`:

```
collection = old_id_to_coll[old_id]
brave_tsm->SetTreeTabNodeCollapsed(collection->node().id(), true)
```

`SetTreeTabNodeCollapsed` updates `TreeTabModel`'s collapsed-ancestor cache so
`DoesBelongToCollapsedNode` works correctly without further fixup.

---

## Save Side — Close Tab and Restore (Ctrl+Z)

### `BravePopulateTreeTabExtraData`

Injected into `BrowserLiveTabContext::GetExtraDataForTab` (called by
`TabRestoreService` when a tab is closed) via the `BRAVE_GET_EXTRA_DATA_FOR_TAB`
macro in `brave/chromium_src/chrome/browser/ui/browser_live_tab_context.cc`.

Walks up from the tab's `GetParentCollection()` to find its
`TreeTabNodeTabCollection`, then writes:
- `brave_tree_node_id` — the node's current ID (stable within this session)
- `brave_tree_parent_node_id` — parent node's ID, or `""` if root

### `BraveRestoreTabTreeHierarchy`

Injected into `BrowserLiveTabContext::AddRestoredTab` via
`BRAVE_ADD_RESTORED_TAB`. After the tab has been re-inserted into the browser,
reads `brave_tree_parent_node_id` from `extra_data` and scans the live strip
for a tab whose `TreeTabNodeTabCollection` has a matching node ID. Then
reparents the restored tab's collection under the found parent.

This lookup by **current** (live) node ID works because the parent tab was NOT
closed — its node ID is stable within the session.

---

## ID Remapping

Unlike `TabGroupId` (which is remapped to a fresh generated ID during restore to
avoid collisions), `TreeTabNodeId` values in the session file are the **old
session's** IDs. `BraveRestoreTreeTabNodeMetadata` uses them as opaque map keys
to match children to parents; fresh IDs were already assigned when the
`TreeTabNodeTabCollection` objects were created during `RestoreTabsToBrowser`.
The `old_id → collection*` map is a one-time translation table discarded after
the restore pass.

---

## Files

### New files

| File | Purpose |
|------|---------|
| `brave/browser/sessions/brave_tree_tab_session_keys.h` | Constant strings for `extra_data` keys |
| `brave/browser/sessions/brave_session_tree_tab_helper.h/.cc` | `TabStripModelObserver` that schedules incremental tree commands |
| `brave/browser/sessions/brave_tree_tab_restore_helper.h/.cc` | Save/restore helpers for `TabRestoreService` (Ctrl+Z) scenario |

### Modified files

| File | Change |
|------|--------|
| `brave/chromium_src/chrome/browser/sessions/session_service.cc` | Defines `BRAVE_SESSION_SERVICE_BUILD_COMMANDS_FOR_TAB`, `BRAVE_SESSION_SERVICE_WINDOW_OPENED`, `BRAVE_SESSION_SERVICE_WINDOW_CLOSING`, `BRAVE_SESSION_SERVICE_DESTRUCTOR`; adds `SetTreeTabNodeData` method; manages `BraveSessionTreeTabHelper` lifetime |
| `chrome/browser/sessions/session_service.cc` | Call-sites for the four Brave macros above |
| `brave/chromium_src/chrome/browser/sessions/session_restore.cc` | Defines `BRAVE_RESTORE_TREE_TAB_NODES` macro |
| `chrome/browser/sessions/session_restore.cc` | Call-site for `BRAVE_RESTORE_TREE_TAB_NODES` |
| `brave/chromium_src/chrome/browser/ui/browser_live_tab_context.cc` | Defines `BRAVE_GET_EXTRA_DATA_FOR_TAB` and `BRAVE_ADD_RESTORED_TAB` macros |
| `brave/browser/sources.gni` | Add new source files |

No changes to `src/components/sessions/core/session_types.h` or
`session_service_commands.h/.cc` are required — the existing
`kCommandAddTabExtraData` infrastructure handles serialization transparently.

---

## Key Invariants

1. **`BRAVE_SESSION_SERVICE_WINDOW_CLOSING` runs while the browser is alive.**
   `WindowClosing` fires before the browser is removed from the browser list.
   `ScheduleResetCommands()` calls `BuildCommandsFromBrowsers()` synchronously,
   so the tree data is captured while all tabs are still accessible.

2. **`SessionService::~SessionService()` flushes to disk.**
   The destructor calls `command_storage_manager()->Save()`, ensuring the rebuild
   buffer populated by `ScheduleResetCommands()` is written before the process
   exits.

3. **`BraveTabStripModel::BuildTreeTabs()` runs before session restore tabs are added.**
   `BraveTabStripModel` is a `Browser` member, constructed before `BrowserView`
   and before `RestoreTabsToBrowser`. Both prefs (`kTreeTabsEnabled`,
   `kVerticalTabsEnabled`) are read synchronously from the already-loaded profile
   in `OnTreeTabRelatedPrefChanged()`, so `tree_model()` is non-null when
   `BRAVE_RESTORE_TREE_TAB_NODES` executes.

4. **Restore uses `tab_visual_index` as a positional key, not a node-ID key.**
   `tab_visual_index` is set to the DFS loop counter in `BuildCommandsForBrowser`.
   After `RestoreTabsToBrowser` adds tabs in the same DFS order,
   `initial_tab_count + tab_visual_index` uniquely addresses each restored tab.

---

## Testing

Browser tests in `brave/browser/sessions/brave_session_restore_browsertest.cc`:

- **Flat tree restored correctly** — single-level parent-child relationship
  survives restart.
- **Multi-level nesting** — three or more levels deep are restored in correct
  order.
- **Collapsed state** — nodes marked collapsed are collapsed after restore; child
  tabs are hidden.
- **Mixed structures** — tree nodes containing groups and splits are restored with
  correct hierarchy.
- **`TreeHierarchyRestoredAfterSessionRestore`** — end-to-end test that creates
  synthetic `SessionTab` objects and calls `BraveRestoreTreeTabNodeMetadata`
  directly to verify reparenting and level calculation.
- **Feature off** — when `kBraveTreeTab` is disabled, no tree `extra_data` is
  written and restore is unaffected.
