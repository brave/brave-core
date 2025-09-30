// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/tabs/public/brave_tab_strip_collection.h"

#include "base/logging.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "brave/components/tabs/public/tree_tab_node_id.h"
#include "components/tabs/public/tab_collection.h"

namespace tabs {

#if !BUILDFLAG(IS_ANDROID)
void BraveTabStripCollection::AddTabRecursive(
    std::unique_ptr<TabInterface> tab,
    size_t index,
    std::optional<tab_groups::TabGroupId> new_group_id,
    bool new_pinned_state) {
  if (!in_tree_tab_mode_) {
    TabStripCollection::AddTabRecursive(std::move(tab), index, new_group_id,
                                        new_pinned_state);
    return;
  }

  // If the previous tab is in the same hierarchy of tree of the opener, we can
  // add the new tab to the same tree node.
  if (const auto* opener = tab->GetOpener(); opener && index > 0) {
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
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace tabs
