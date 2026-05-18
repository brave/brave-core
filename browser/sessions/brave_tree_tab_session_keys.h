// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_SESSIONS_BRAVE_TREE_TAB_SESSION_KEYS_H_
#define BRAVE_BROWSER_SESSIONS_BRAVE_TREE_TAB_SESSION_KEYS_H_

// Keys used in SessionTab::extra_data to persist tree-tab node structure.
// All values are stored as plain strings.

// The TreeTabNodeId of the tree node that owns this tab, serialized as a
// base::Token 128-bit hex string.
inline constexpr char kBraveTreeNodeIdKey[] = "brave_tree_node_id";

// The TreeTabNodeId of the parent tree node, serialized the same way.
// An empty string means this node is a root (top-level) node.
inline constexpr char kBraveTreeParentNodeIdKey[] =
    "brave_tree_parent_node_id";

// "1" if the tree node owning this tab is collapsed, "0" otherwise.
// Only written for tabs that are the direct current_value (kTab type) of
// their TreeTabNodeTabCollection.
inline constexpr char kBraveTreeNodeCollapsedKey[] =
    "brave_tree_node_collapsed";

#endif  // BRAVE_BROWSER_SESSIONS_BRAVE_TREE_TAB_SESSION_KEYS_H_
