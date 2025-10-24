// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/tabs/brave_tree_tab_strip_collection_delegate.h"

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "brave/browser/ui/tabs/tree_tab_model.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "components/tabs/public/unpinned_tab_collection.h"

BraveTreeTabStripCollectionDelegate::BraveTreeTabStripCollectionDelegate(
    tabs::BraveTabStripCollection& collection,
    base::WeakPtr<TreeTabModel> tree_tab_model)
    : BraveTabStripCollectionDelegate(collection),
      tree_tab_model_(tree_tab_model) {
  CHECK(tree_tab_model_);
  auto* unpinned_collection = collection_->unpinned_collection();
  CHECK(unpinned_collection);
  tabs::TreeTabNodeTabCollection::BuildTreeTabs(
      *unpinned_collection,
      base::BindRepeating(&TreeTabModel::AddTreeTabNode, tree_tab_model_));
}

BraveTreeTabStripCollectionDelegate::~BraveTreeTabStripCollectionDelegate() {
  in_desturction_ = true;

  auto* unpinned_collection = collection_->unpinned_collection();
  CHECK(unpinned_collection);
  tabs::TreeTabNodeTabCollection::FlattenTreeTabs(
      *unpinned_collection,
      base::BindRepeating(&TreeTabModel::RemoveTreeTabNode, tree_tab_model_));
}

bool BraveTreeTabStripCollectionDelegate::ShouldHandleTabManipulation() const {
  return !in_desturction_;
}

void BraveTreeTabStripCollectionDelegate::AddTabRecursive(
    std::unique_ptr<tabs::TabInterface> tab,
    size_t index,
    std::optional<tab_groups::TabGroupId> new_group_id,
    bool new_pinned_state,
    tabs::TabInterface* opener) const {
  // If the previous tab is in the same hierarchy of tree of the opener, we can
  // add the new tab to the same tree node.
  if (opener && index > 0) {
    auto* opener_collection =
        collection_->GetParentCollection(opener, GetPassKey());
    CHECK_EQ(opener_collection->type(), tabs::TabCollection::Type::TREE_NODE);

    tabs::TabInterface* previous_tab =
        collection_->GetTabAtIndexRecursive(index - 1);

    auto* previous_tab_collection =
        collection_->GetParentCollection(previous_tab, GetPassKey());
    CHECK_EQ(previous_tab_collection->type(),
             tabs::TabCollection::Type::TREE_NODE);

    if (static_cast<tabs::TreeTabNodeTabCollection*>(opener_collection)
            ->GetTopLevelAncestor() ==
        static_cast<tabs::TreeTabNodeTabCollection*>(previous_tab_collection)
            ->GetTopLevelAncestor()) {
      auto tree_tab_node = std::make_unique<tabs::TreeTabNodeTabCollection>(
          tree_tab::TreeTabNodeId::GenerateNew(), std::move(tab));
      CHECK(tree_tab_model_);
      tree_tab_model_->AddTreeTabNode(tree_tab_node->node());

      opener_collection->AddCollection(std::move(tree_tab_node),
                                       opener_collection->ChildCount());
      return;
    }
  }

  // Otherwise, we insert the new tab into the current collection and then wrap
  // it with a tabs::TreeTabNodeTabCollection.
  auto* added_tab = tab.get();
  collection_->AddTabRecursive(std::move(tab), index, new_group_id,
                                   new_pinned_state, GetPassKey());

  auto* parent_collection =
      collection_->GetParentCollection(added_tab, GetPassKey());

  CHECK(parent_collection);
  CHECK_NE(parent_collection->type(), tabs::TabCollection::Type::TREE_NODE);
  auto target_index = parent_collection->GetIndexOfTab(added_tab);
  CHECK(target_index);

  auto detached_tab = parent_collection->MaybeRemoveTab(added_tab);
  auto tree_tab_node = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(), std::move(detached_tab));
  CHECK(tree_tab_model_);
  tree_tab_model_->AddTreeTabNode(tree_tab_node->node());
  parent_collection->AddCollection(std::move(tree_tab_node), *target_index);
}
