// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/sessions/brave_tree_tab_restore_helper.h"

#include <map>
#include <string>

#include "brave/browser/sessions/brave_tree_tab_session_keys.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/features.h"
#include "components/sessions/core/session_types.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"

namespace {

// Returns the nearest TREE_NODE ancestor collection for |tab_iface|, or
// nullptr if the tab is pinned or the tree feature is inactive.
tabs::TreeTabNodeTabCollection* FindTreeNodeCollection(
    tabs::TabInterface* tab_iface) {
  const tabs::TabCollection* parent = tab_iface->GetParentCollection();
  while (parent) {
    if (parent->type() == tabs::TabCollection::Type::TREE_NODE) {
      return const_cast<tabs::TreeTabNodeTabCollection*>(
          static_cast<const tabs::TreeTabNodeTabCollection*>(parent));
    }
    if (parent->type() == tabs::TabCollection::Type::UNPINNED ||
        parent->type() == tabs::TabCollection::Type::PINNED ||
        parent->type() == tabs::TabCollection::Type::TABSTRIP) {
      break;
    }
    parent = parent->GetParentCollection();
  }
  return nullptr;
}

}  // namespace

void BraveRestoreTreeTabNodeMetadata(
    Browser* browser,
    const std::vector<std::unique_ptr<sessions::SessionTab>>& session_tabs,
    int initial_tab_count) {
  if (!base::FeatureList::IsEnabled(tabs::kBraveTreeTab)) {
    return;
  }

  auto* brave_tsm =
      static_cast<BraveTabStripModel*>(browser->tab_strip_model());
  if (!brave_tsm->tree_model()) {
    return;
  }

  // Build a map from the serialised old node-id string to the live
  // TreeTabNodeTabCollection that was created for that tab during restore.
  //
  // After RestoreTabsToBrowser all nodes are root-level (flat). We use the
  // tab_visual_index stored in each SessionTab to locate the corresponding
  // live WebContents in the browser, then walk up to its TREE_NODE parent.
  std::map<std::string, tabs::TreeTabNodeTabCollection*> old_id_to_coll;

  for (const auto& session_tab : session_tabs) {
    auto node_it = session_tab->extra_data.find(kBraveTreeNodeIdKey);
    if (node_it == session_tab->extra_data.end() || node_it->second.empty()) {
      continue;
    }

    int browser_index =
        initial_tab_count + session_tab->tab_visual_index;
    if (browser_index < 0 ||
        browser_index >= browser->tab_strip_model()->count()) {
      continue;
    }

    content::WebContents* wc =
        browser->tab_strip_model()->GetWebContentsAt(browser_index);
    if (!wc) {
      continue;
    }

    tabs::TabInterface* tab_iface =
        browser->tab_strip_model()->GetTabForWebContents(wc);
    if (!tab_iface) {
      continue;
    }

    tabs::TreeTabNodeTabCollection* tree_coll =
        FindTreeNodeCollection(tab_iface);
    if (!tree_coll) {
      continue;
    }

    // Multiple tabs may map to the same node (group/split current_value).
    old_id_to_coll.emplace(node_it->second, tree_coll);
  }

  // Reparent in tab_visual_index order (which is DFS order for the tree).
  // Processing in DFS order guarantees a parent collection exists in
  // old_id_to_coll before any of its children are processed.
  for (const auto& session_tab : session_tabs) {
    auto node_it = session_tab->extra_data.find(kBraveTreeNodeIdKey);
    auto parent_it =
        session_tab->extra_data.find(kBraveTreeParentNodeIdKey);

    if (node_it == session_tab->extra_data.end() || node_it->second.empty()) {
      continue;
    }
    if (parent_it == session_tab->extra_data.end() ||
        parent_it->second.empty()) {
      continue;  // Root node – no reparenting needed.
    }

    auto child_map_it = old_id_to_coll.find(node_it->second);
    auto parent_map_it = old_id_to_coll.find(parent_it->second);
    if (child_map_it == old_id_to_coll.end() ||
        parent_map_it == old_id_to_coll.end()) {
      continue;
    }

    tabs::TreeTabNodeTabCollection* child_coll = child_map_it->second;
    tabs::TreeTabNodeTabCollection* parent_coll = parent_map_it->second;

    if (child_coll->GetParentCollection() == parent_coll) {
      continue;  // Already correctly parented (e.g. duplicate session_tab).
    }

    // Remove child from its current parent (the flat UnpinnedTabCollection).
    tabs::TabCollection* current_parent = child_coll->GetParentCollection();
    std::unique_ptr<tabs::TabCollection> owned_child =
        current_parent->MaybeRemoveCollection(child_coll);
    if (!owned_child) {
      continue;
    }

    // Append as the last child of the target parent. Since we process tabs
    // in DFS (visual) order, parent nodes are always finalized before their
    // children, so ChildCount() gives the correct append position.
    size_t target_index = parent_coll->ChildCount();
    parent_coll->AddCollection<tabs::TreeTabNodeTabCollection>(
        std::unique_ptr<tabs::TreeTabNodeTabCollection>(
            static_cast<tabs::TreeTabNodeTabCollection*>(
                owned_child.release())),
        target_index);
  }

  // Apply collapsed state. Must happen after reparenting so that the
  // collapsed-ancestor cache in TreeTabModel is populated correctly.
  for (const auto& session_tab : session_tabs) {
    auto node_it = session_tab->extra_data.find(kBraveTreeNodeIdKey);
    auto collapsed_it =
        session_tab->extra_data.find(kBraveTreeNodeCollapsedKey);

    if (node_it == session_tab->extra_data.end() || node_it->second.empty()) {
      continue;
    }
    if (collapsed_it == session_tab->extra_data.end() ||
        collapsed_it->second != "1") {
      continue;
    }

    auto coll_it = old_id_to_coll.find(node_it->second);
    if (coll_it == old_id_to_coll.end()) {
      continue;
    }

    brave_tsm->SetTreeTabNodeCollapsed(coll_it->second->node().id(), true);
  }
}
