# Tree Tabs Session Restore

## Overview

This document describes the design for persisting Brave's `TreeTabNode`
hierarchy across browser sessions. Two distinct scenarios require handling:

1. **Browser restart** — closing all windows and reopening; uses
   `SessionService`.
2. **Close-tab-and-restore** — Ctrl+t / History > Recently Closed; uses
   `TabRestoreService`.

See [tree_tabs_architecture.md](tree_tabs_architecture.md) for background on the
`TreeTabNode` and `TreeTabNodeTabCollection` model.

---

## Background: How Tab Groups and Split Tabs Are Persisted

The session service already persists two kinds of tab structural metadata — tab
groups and split tabs — using a layered approach:

1. **[`SessionTab::group`](https://source.chromium.org/chromium/chromium/src/+/main:components/sessions/core/session_types.h;l=80;drc=fd58fdc82b0c8b612a2b06be36cf35829dda13cc)**
   /
   **[`SessionTab::split_id`](https://source.chromium.org/chromium/chromium/src/+/main:components/sessions/core/session_types.h;l=83;drc=fd58fdc82b0c8b612a2b06be36cf35829dda13cc)**
   — per-tab membership fields in
   [`session_types.h`](https://source.chromium.org/chromium/chromium/src/+/main:components/sessions/core/session_types.h;l=83;drc=fd58fdc82b0c8b612a2b06be36cf35829dda13cc).
   These are id of group and split tab respectively.
2. **[`SessionTabGroup`](https://source.chromium.org/chromium/chromium/src/+/main:components/sessions/core/session_types.h;l=122;drc=fd58fdc82b0c8b612a2b06be36cf35829dda13cc)**
   /
   **[`SessionSplitTab`](https://source.chromium.org/chromium/chromium/src/+/main:components/sessions/core/session_types.h;l=147;drc=fd58fdc82b0c8b612a2b06be36cf35829dda13cc)**
   structs in
   [`SessionWindow`](https://source.chromium.org/chromium/chromium/src/+/main:components/sessions/core/session_types.h;l=166;drc=fd58fdc82b0c8b612a2b06be36cf35829dda13cc)
   — hold per-group/split visual metadata.
3. **[Session commands](https://source.chromium.org/chromium/chromium/src/+/main:components/sessions/core/session_service_commands.cc)**

   `kCommandSetTabGroup`, `kCommandSetTabGroupMetadata2`, `kCommandSetSplitTab`,
   `kCommandSetSplitTabData`

   Defined in session_commands.cc internally and exposes methods to create
   theses commands. And
   [`SessionService::BuildCommandsForTab](https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/sessions/session_service.cc;l=629;drc=10be28b2c5e3a78f4e74d1841fc5f16aae2ab3ba)/[Browser](https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/sessions/session_service_base.cc;l=736;drc=a0f96f45d931f3fc57126fad5d8050ea63bd2d25)`
   will use these commands for full rebuilds and incrementally on live changes.

4. **[`RestoreSessionFromCommands`](https://source.chromium.org/chromium/chromium/src/+/main:components/sessions/core/session_service_commands.h;l=142;drc=f9d29b4ce34d252f7a124d326331901c3b942ee5)**
   — replays commands and populates `SessionTab::group` and
   `SessionWindow::tab_groups` in memory.
5. **[`RestoreTabsToBrowser`](https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/sessions/session_restore.cc;l=901;drc=815c5cff075fb58c5ec93d967479af57312a5894)
   →
   [`RestoreTabGroupMetadata`](https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/sessions/session_restore.cc;l=1073;drc=815c5cff075fb58c5ec93d967479af57312a5894)**
   — rebuilds group structures in the live tab strip, remapping old session IDs
   to fresh ones.

Tree tab restoration follows the same five-layer pattern, with one key
difference: since `tree_tab::TreeTabNodeId` is a Brave-specific type, we use the
existing
**[`kCommandAddTabExtraData`](https://source.chromium.org/chromium/chromium/src/+/main:components/sessions/core/session_service_commands.h;l=111;drc=f9d29b4ce34d252f7a124d326331901c3b942ee5)**
mechanism (id=33) rather than adding new fields to Chromium's `session_types.h`.

---

## Data Stored Per Tab

Three keys are written into `SessionTab::extra_data` (and
`tab_restore::Tab::extra_data`) for each tab in tree mode:

| Key                         | Value                                         | Notes                                                                                                                                                           |
| --------------------------- | --------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `brave_tree_node_id`        | `TreeTabNodeId` serialized as base::Token hex | The node this tab belongs to                                                                                                                                    |
| `brave_tree_parent_node_id` | Parent's token hex, or `""` for a root node   | Encodes the parent-child edge                                                                                                                                   |
| `brave_tree_node_collapsed` | `"1"` or `"0"`                                | Written only on the primary tab of each node (the tab that is the direct `current_value` of the `TreeTabNodeTabCollection`, i.e. `current_value_type_ == kTab`) |

For tree nodes whose `current_value` is a `TabGroupTabCollection` or
`SplitTabCollection`, `brave_tree_node_id` and `brave_tree_parent_node_id` are
written on the first tab in the group/split, and `brave_tree_node_collapsed` is
omitted (group/split nodes cannot be individually collapsed today).

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                  SAVE PATH — Browser Shutdown                       │
│                                                                     │
│  BraveBrowser::OnTreeTabChanged (TabStripModelObserver)             │
│    kNodeCreated          → UpdateTreeTabSessionDataForNode          │
│    kNodeReparented       → UpdateTreeTabSessionDataForNode          │
│    kNodeCollapsedStateChanged → UpdateTreeTabCollapsedState         │
│    → SessionService::AddTabExtraData (public Chromium API)          │
│    → ScheduleCommand(CreateAddTabExtraDataCommand(...))             │
│              │                                                      │
│  SessionServiceBase::BuildCommandsForBrowser (periodic rebuild)     │
│    loop per tab → BuildCommandsForTab(...)                          │
│              → BRAVE_BUILD_COMMANDS_FOR_TREE_TAB                    │
│              → AppendRebuildCommand(CreateAddTabExtraDataCommand)   │
│              │                                                      │
│  SessionService::~SessionService → Save()                           │
│    → flushes pending commands to disk                               │
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
│    3. Apply collapsed state via                                     |
│    BraveTabStripModel::SetTreeTabNodeCollapsed                      │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│               SAVE PATH — Close Tab (Ctrl+w)                        │
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
│             RESTORE PATH — Restore (Ctrl+Shift+t)                   │
│                                                                     │
│  BrowserLiveTabContext::AddRestoredTab                              │
│  (BRAVE_ADD_RESTORED_TAB chromium_src override)                     │
│    → BraveRestoreTabTreeHierarchy                                   │
│      scans live strip for parent by its current node ID             │
│      → MaybeRemoveCollection + AddCollection to reparent            │
└─────────────────────────────────────────────────────────────────────┘
```
