# Tree Tabs Session Restore

## Overview

This document describes the design for persisting Brave's `TreeTabNode` hierarchy
across browser sessions. When a browser restarts, the session service must restore
not only the flat list of tabs but also the parent-child tree structure and collapsed
state of each node.

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

Three keys are written into `SessionTab::extra_data` for each tab in tree mode:

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
│                          SAVE PATH                                  │
│                                                                     │
│  BraveTreeTabStripCollectionDelegate                                │
│    on_create / on_move callbacks                                    │
│              │                                                      │
│              ▼                                                      │
│  BraveSessionTreeTabHelper                                          │
│    ScheduleCommand(CreateAddTabExtraDataCommand(...))  ──────────┐  │
│                                                                  │  │
│  SessionService::BuildCommandsForTab (chromium_src override)     │  │
│    also emits three CreateAddTabExtraDataCommand calls per tab ──┘  │
│              │                                                      │
│              ▼                                                      │
│          Session file on disk                                       │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                        RESTORE PATH                                 │
│                                                                     │
│  RestoreSessionFromCommands                                         │
│    kCommandAddTabExtraData → SessionTab::extra_data populated       │
│              │                                                      │
│              ▼                                                      │
│  RestoreTabsToBrowser  (tabs added flat, each in its own            │
│    TreeTabNodeTabCollection with a fresh generated ID)              │
│              │                                                      │
│              ▼                                                      │
│  RestoreTabGroupMetadata  (existing, unchanged)                     │
│              │                                                      │
│              ▼                                                      │
│  BraveRestoreTreeTabNodeMetadata  (NEW)                             │
│    1. Build old_id → TreeTabNodeTabCollection* map                  │
│    2. Reparent collections to recreate tree hierarchy               │
│    3. Apply collapsed state via TreeTabModel::SetCollapsed          │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Save Side

### Full Rebuild (`BuildCommandsForTab`)

`SessionService::BuildCommandsForTab` is overridden in
`brave/chromium_src/chrome/browser/sessions/session_service.cc`. When
`kBraveTreeTab` is enabled, after the standard commands are emitted for a tab,
three additional `CreateAddTabExtraDataCommand` calls are made using data looked
up from `BraveTabStripCollection`:

```cpp
// Pseudocode for the Brave override of BuildCommandsForTab
if (base::FeatureList::IsEnabled(features::kBraveTreeTab)) {
  auto* collection = GetTreeTabNodeCollectionForTab(tab_id, browser);
  if (collection) {
    const auto& node_id = collection->node().id();
    ScheduleCommand(CreateAddTabExtraDataCommand(
        tab_id, "brave_tree_node_id", node_id.token().ToString()));

    auto* parent = GetParentTreeTabNodeCollection(collection);
    ScheduleCommand(CreateAddTabExtraDataCommand(
        tab_id, "brave_tree_parent_node_id",
        parent ? parent->node().id().token().ToString() : ""));

    if (collection->current_value_type() ==
        TreeTabNodeTabCollection::CurrentValueType::kTab) {
      ScheduleCommand(CreateAddTabExtraDataCommand(
          tab_id, "brave_tree_node_collapsed",
          collection->node().collapsed() ? "1" : "0"));
    }
  }
}
```

### Incremental Updates (`BraveSessionTreeTabHelper`)

`BraveSessionTreeTabHelper` is a new class that observes
`BraveTreeTabStripCollectionDelegate` callbacks for the lifetime of a browser
window:

- **`on_create`** — schedules the three `extra_data` commands for the new tab.
- **`on_move`** — schedules updated `brave_tree_parent_node_id` for the moved
  tab (and any of its children whose parent also changed).
- **Collapsed toggle** — `TreeTabModel::SetCollapsed` notifies observers;
  `BraveSessionTreeTabHelper` listens via
  `RegisterAddTreeTabNodeCallback`/`RegisterWillRemoveTreeTabNodeCallback` and
  schedules an updated `brave_tree_node_collapsed` command.

---

## Restore Side

### After `RestoreTabGroupMetadata`

A new function `BraveRestoreTreeTabNodeMetadata(Browser*, SessionWindow&)` is
called from the `chromium_src` override of `session_restore.cc`, immediately
after the existing `RestoreTabGroupMetadata` call.

#### Step 1 — Build the old-ID-to-collection map

For each restored tab in the browser's tab strip:

```
old_node_id = tab->extra_data["brave_tree_node_id"]   (deserialized token)
collection  = tab->GetParentCollection()               (TreeTabNodeTabCollection*)
old_to_collection[old_node_id] = collection
```

If `GetParentCollection()` returns a `TabGroupTabCollection` or
`SplitTabCollection` (because the tab is inside a group/split that is itself
inside a tree node), walk up via `GetParentTreeNodeCollectionOfTab` from
`BraveTreeTabStripCollectionDelegate` to find the nearest ancestor
`TreeTabNodeTabCollection`.

#### Step 2 — Reparent collections

Iterate tabs in their saved `tab_visual_index` order. For each tab whose
`brave_tree_parent_node_id` is non-empty:

```
parent_collection = old_to_collection[parent_old_id]
child_collection  = old_to_collection[old_id]
// Move child_collection under parent_collection using TabCollection APIs.
```

Processing in visual order ensures parent collections exist before their children
are reparented.

#### Step 3 — Restore collapsed state

For each tab that carries `brave_tree_node_collapsed = "1"`:

```
collection = old_to_collection[old_node_id]
tree_tab_model->SetCollapsed(collection->node().id(), true)
```

`TreeTabModel::SetCollapsed` handles updating the collapsed-ancestor cache so
that `DoesBelongToCollapsedNode` works correctly without further fixup.

---

## ID Remapping

Unlike `TabGroupId` (which is remapped to a fresh generated ID during restore to
avoid collisions), `TreeTabNodeId` is already generated fresh by
`AddTabRecursive` during restore. The `old_id → collection*` map built in Step 1
is a one-time translation table used only during the restore pass; it does not
need to be retained after `BraveRestoreTreeTabNodeMetadata` returns.

---

## Files

### New files

- `brave/browser/sessions/brave_session_tree_tab_helper.h`
- `brave/browser/sessions/brave_session_tree_tab_helper.cc`

### Modified files

| File | Change |
|------|--------|
| `brave/chromium_src/chrome/browser/sessions/session_service.cc` | Override `BuildCommandsForTab` to emit tree `extra_data` commands; instantiate `BraveSessionTreeTabHelper` per browser |
| `brave/chromium_src/chrome/browser/sessions/session_restore.cc` | Call `BraveRestoreTreeTabNodeMetadata` after `RestoreTabGroupMetadata` |
| `brave/browser/sessions/BUILD.gn` | Add new sources and `//brave/components/tabs/public:tree_tab_node` dep |

No changes to `src/components/sessions/core/session_types.h` or
`session_service_commands.h/.cc` are required — the existing
`kCommandAddTabExtraData` infrastructure handles serialization transparently.

---

## Testing

New browser tests in `brave/browser/sessions/brave_session_restore_browsertest.cc`:

- **Flat tree restored correctly** — single-level parent-child relationship
  survives restart.
- **Multi-level nesting** — three or more levels deep are restored in correct
  order.
- **Collapsed state** — nodes marked collapsed are collapsed after restore; child
  tabs are hidden.
- **Mixed structures** — tree nodes containing groups and splits are restored with
  correct hierarchy.
- **Feature off** — when `kBraveTreeTab` is disabled, no tree `extra_data` is
  written and restore is unaffected.
