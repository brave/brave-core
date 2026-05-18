// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/sessions/brave_tab_restore_tree_helper.h"

#include <string>

#include "brave/browser/sessions/brave_tree_tab_session_keys.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/features.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"

namespace {

// Returns the nearest TREE_NODE ancestor of |tab_iface|, or nullptr if the tab
// is pinned or tree tabs are not active.
const tabs::TreeTabNodeTabCollection* FindTreeNodeCollectionForTab(
    const tabs::TabInterface* tab_iface) {
  const tabs::TabCollection* parent = tab_iface->GetParentCollection();
  while (parent) {
    if (parent->type() == tabs::TabCollection::Type::TREE_NODE) {
      return static_cast<const tabs::TreeTabNodeTabCollection*>(parent);
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

void BravePopulateTreeTabExtraData(
    TabStripModel* tab_strip_model,
    int index,
    std::map<std::string, std::string>* extra_data) {
  if (!base::FeatureList::IsEnabled(tabs::kBraveTreeTab)) {
    return;
  }
  auto* brave_tsm = static_cast<BraveTabStripModel*>(tab_strip_model);
  if (!brave_tsm->tree_model()) {
    return;
  }
  auto* tab_iface = brave_tsm->GetTabAtIndex(index);
  if (!tab_iface) {
    return;
  }
  const auto* tree_coll = FindTreeNodeCollectionForTab(tab_iface);
  if (!tree_coll) {
    return;
  }
  (*extra_data)[kBraveTreeNodeIdKey] = tree_coll->node().id().ToString();
  auto parent_id = tree_coll->node().GetParentTreeNodeId();
  (*extra_data)[kBraveTreeParentNodeIdKey] =
      parent_id ? parent_id->ToString() : "";
}

void BraveRestoreTabTreeHierarchy(
    Browser* browser,
    content::WebContents* restored_wc,
    const std::map<std::string, std::string>& extra_data) {
  if (!base::FeatureList::IsEnabled(tabs::kBraveTreeTab)) {
    return;
  }
  auto* brave_tsm =
      static_cast<BraveTabStripModel*>(browser->tab_strip_model());
  if (!brave_tsm->tree_model()) {
    return;
  }

  // Only reparent if the saved data describes a child node (non-empty parent).
  auto parent_id_it = extra_data.find(kBraveTreeParentNodeIdKey);
  if (parent_id_it == extra_data.end() || parent_id_it->second.empty()) {
    return;
  }
  const std::string& target_parent_node_id = parent_id_it->second;

  // Locate the restored tab in the strip.
  auto* restored_tab = brave_tsm->GetTabForWebContents(restored_wc);
  if (!restored_tab) {
    return;
  }
  auto* child_coll = const_cast<tabs::TreeTabNodeTabCollection*>(
      FindTreeNodeCollectionForTab(restored_tab));
  if (!child_coll) {
    return;
  }

  // Scan the strip to find the live TreeTabNodeTabCollection whose node ID
  // matches the saved parent node ID. Because the parent tab was not closed
  // (only the child was), its node ID is stable within this session.
  tabs::TreeTabNodeTabCollection* parent_coll = nullptr;
  for (int i = 0; i < brave_tsm->count(); ++i) {
    auto* candidate = brave_tsm->GetTabAtIndex(i);
    if (!candidate || candidate == restored_tab) {
      continue;
    }
    const auto* coll = FindTreeNodeCollectionForTab(candidate);
    if (!coll) {
      continue;
    }
    if (coll->node().id().ToString() == target_parent_node_id) {
      parent_coll = const_cast<tabs::TreeTabNodeTabCollection*>(coll);
      break;
    }
  }
  if (!parent_coll) {
    return;  // Parent tab was also closed; nothing to reparent under.
  }

  if (child_coll->GetParentCollection() == parent_coll) {
    return;  // Already correctly nested (shouldn't happen, but guard anyway).
  }

  // Remove child from its current flat parent (UnpinnedTabCollection) and
  // append it as the last child of the target parent tree node.
  auto* current_parent = child_coll->GetParentCollection();
  std::unique_ptr<tabs::TabCollection> owned =
      current_parent->MaybeRemoveCollection(child_coll);
  if (!owned) {
    return;
  }
  parent_coll->AddCollection<tabs::TreeTabNodeTabCollection>(
      std::unique_ptr<tabs::TreeTabNodeTabCollection>(
          static_cast<tabs::TreeTabNodeTabCollection*>(owned.release())),
      parent_coll->ChildCount());
}
