// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/tabs/public/brave_tab_strip_collection.h"

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/notimplemented.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "brave/components/tabs/public/tree_tab_node_id.h"
#include "components/tabs/public/tab_collection.h"

namespace tabs {

BraveTabStripCollection::BraveTabStripCollection() {
  CHECK(base::FeatureList::IsEnabled(tabs::features::kBraveTreeTab));
}

void BraveTabStripCollection::AddTabRecursive(
    std::unique_ptr<TabInterface> tab,
    size_t index,
    std::optional<tab_groups::TabGroupId> new_group_id,
    bool new_pinned_state,
    TabInterface* opener) {
  if (!in_tree_tab_mode_) {
    TabStripCollection::AddTabRecursive(std::move(tab), index, new_group_id,
                                        new_pinned_state);
    return;
  }

  // If the previous tab is in the same hierarchy of tree of the opener, we can
  // add the new tab to the same tree node.
  if (opener && index > 0) {
    auto* opener_collection = opener->GetParentCollection(GetPassKey());
    CHECK_EQ(opener_collection->type(), TabCollection::Type::TREE_NODE);

    TabInterface* previous_tab = GetTabAtIndexRecursive(index - 1);

    auto* previous_tab_collection =
        previous_tab->GetParentCollection(GetPassKey());
    CHECK_EQ(previous_tab_collection->type(), TabCollection::Type::TREE_NODE);

    if (static_cast<TreeTabNode*>(opener_collection)->GetTopLevelAncestor() ==
        static_cast<TreeTabNode*>(previous_tab_collection)
            ->GetTopLevelAncestor()) {
      auto tree_tab_node = std::make_unique<TreeTabNode>(
          tree_tab::TreeTabNodeId::GenerateNew(), std::move(tab));

      opener_collection->AddCollection(std::move(tree_tab_node),
                                       opener_collection->ChildCount());
      return;
    }
  }

  // Otherwise, we insert the new tab into the current collection and then wrap
  // it with a TreeTabNode.
  auto* added_tab = tab.get();
  TabStripCollection::AddTabRecursive(std::move(tab), index, new_group_id,
                                      new_pinned_state);
  auto* parent_collection = added_tab->GetParentCollection(GetPassKey());
  CHECK(parent_collection);
  CHECK_NE(parent_collection->type(), TabCollection::Type::TREE_NODE);
  auto target_index = parent_collection->GetIndexOfTab(added_tab);
  CHECK(target_index);

  auto detached_tab = parent_collection->MaybeRemoveTab(added_tab);
  auto tree_tab_node = std::make_unique<TreeTabNode>(
      tree_tab::TreeTabNodeId::GenerateNew(), std::move(detached_tab));
  parent_collection->AddCollection(std::move(tree_tab_node), *target_index);
}

void BraveTabStripCollection::MoveTabRecursive(
    size_t initial_index,
    size_t final_index,
    std::optional<tab_groups::TabGroupId> new_group_id,
    bool new_pinned_state) {
  if (!in_tree_tab_mode_) {
    TabStripCollection::MoveTabRecursive(initial_index, final_index,
                                         new_group_id, new_pinned_state);
    return;
  }

  // TODO(https://github.com/brave/brave-browser/issues/49790) Handle tree tab
  // move properly.
  NOTIMPLEMENTED();
  TabStripCollection::MoveTabRecursive(initial_index, final_index, new_group_id,
                                       new_pinned_state);
}

void BraveTabStripCollection::MoveTabsRecursive(
    const std::vector<int>& tab_indices,
    size_t destination_index,
    std::optional<tab_groups::TabGroupId> new_group_id,
    bool new_pinned_state,
    const std::set<TabCollection::Type>& retain_collection_types) {
  if (!in_tree_tab_mode_) {
    TabStripCollection::MoveTabsRecursive(tab_indices, destination_index,
                                          new_group_id, new_pinned_state,
                                          retain_collection_types);
    return;
  }

  // TODO(https://github.com/brave/brave-browser/issues/49790) Handle tree tab
  // move properly.
  NOTIMPLEMENTED();
  TabStripCollection::MoveTabsRecursive(tab_indices, destination_index,
                                        new_group_id, new_pinned_state,
                                        retain_collection_types);
}

std::unique_ptr<TabInterface>
BraveTabStripCollection::RemoveTabAtIndexRecursive(size_t index) {
  if (!in_tree_tab_mode_) {
    return TabStripCollection::RemoveTabAtIndexRecursive(index);
  }

  // TODO(https://github.com/brave/brave-browser/issues/49789) Handle tree tab
  // removal properly.
  NOTIMPLEMENTED();
  return TabStripCollection::RemoveTabAtIndexRecursive(index);
}

std::unique_ptr<TabInterface> BraveTabStripCollection::RemoveTabRecursive(
    TabInterface* tab,
    bool close_empty_group_collection) {
  if (!in_tree_tab_mode_) {
    return TabStripCollection::RemoveTabRecursive(tab,
                                                  close_empty_group_collection);
  }

  // TODO(https://github.com/brave/brave-browser/issues/49789) Handle tree tab
  // removal properly.
  NOTIMPLEMENTED();
  return TabStripCollection::RemoveTabRecursive(tab,
                                                close_empty_group_collection);
}

}  // namespace tabs
