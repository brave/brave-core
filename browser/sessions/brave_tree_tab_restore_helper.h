// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_SESSIONS_BRAVE_TREE_TAB_RESTORE_HELPER_H_
#define BRAVE_BROWSER_SESSIONS_BRAVE_TREE_TAB_RESTORE_HELPER_H_

#include <memory>
#include <vector>

class Browser;

namespace sessions {
struct SessionTab;
}  // namespace sessions

// Reconstructs the tree-tab parent-child hierarchy and collapsed state for a
// just-restored browser window. Must be called after RestoreTabGroupMetadata
// so that group/split memberships are already set up in the tab strip.
//
// |session_tabs| is the flat list of SessionTab entries from the saved window.
// |initial_tab_count| is the number of tabs that were already in the browser
// before restoration began; this is used to map tab_visual_index values to
// their current positions in the browser's tab strip.
void BraveRestoreTreeTabNodeMetadata(
    Browser* browser,
    const std::vector<std::unique_ptr<sessions::SessionTab>>& session_tabs,
    int initial_tab_count);

#endif  // BRAVE_BROWSER_SESSIONS_BRAVE_TREE_TAB_RESTORE_HELPER_H_
