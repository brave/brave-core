// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/tabs/brave_tree_tab_strip_collection_delegate.h"

#include <variant>

#include "base/functional/bind.h"
#include "base/notimplemented.h"
#include "brave/browser/ui/tabs/tree_tab_model.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/unpinned_tab_collection.h"
#include "third_party/abseil-cpp/absl/functional/overload.h"

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
      base::BindRepeating(&TreeTabModel::AddTreeTabNode, tree_tab_model_),
      base::BindRepeating(&TreeTabModel::RemoveTreeTabNode, tree_tab_model_));
}

BraveTreeTabStripCollectionDelegate::~BraveTreeTabStripCollectionDelegate() {
  in_destruction_ = true;

  auto* unpinned_collection = collection_->unpinned_collection();
  CHECK(unpinned_collection);
  tabs::TreeTabNodeTabCollection::FlattenTreeTabs(*unpinned_collection);
}

bool BraveTreeTabStripCollectionDelegate::ShouldHandleTabManipulation() const {
  return !in_destruction_;
}

void BraveTreeTabStripCollectionDelegate::AddTabRecursive(
    std::unique_ptr<tabs::TabInterface> tab,
    size_t index,
    std::optional<tab_groups::TabGroupId> new_group_id,
    bool new_pinned_state,
    tabs::TabInterface* opener) const {
  if (new_pinned_state) {
    // Just add the tab to pinned collection without tree tab structure.
    collection_->AddTabRecursive(std::move(tab), index, new_group_id,
                                 new_pinned_state, GetPassKey());
    return;
  }

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
      // Now opener and previous tab are in the same tree node, we can add the
      // new tab to the same tree node.
      // Find insertion index among the opener collection. - the |index| is
      // recursively calculated one, so we should calculate the target index
      // based on the opener collection.
      const auto opener_index = *collection_->GetIndexOfTabRecursive(
          static_cast<tabs::TreeTabNodeTabCollection*>(opener_collection)
              ->current_tab()
              .get());
      auto target_index = 0;
      auto tab_count = 0;

      for (auto& child : collection_->GetChildrenForDelegate(*opener_collection,
                                                             GetPassKey())) {
        if (opener_index + tab_count == index) {
          break;
        }

        CHECK(opener_index + tab_count < index)
            << "We don't assume target index exceed the specified index.";

        target_index++;
        std::visit(
            absl::Overload{
                [&](const std::unique_ptr<tabs::TabInterface>& tab) {
                  tab_count++;
                },
                [&](const std::unique_ptr<tabs::TabCollection>& collection) {
                  tab_count += collection->TabCountRecursive();
                }},
            child);
      }

      // Check if specified |index| matches the calculated target index.
      // When opening a empty new tab, it is possible that they don't match.
      // In that case, we just add the new tab to the unpinned collection from
      // the code below.
      if (opener_index + tab_count == index) {
        auto* tab_ptr = tab.get();
        auto tree_tab_node = std::make_unique<tabs::TreeTabNodeTabCollection>(
            tree_tab::TreeTabNodeId::GenerateNew(), std::move(tab),
            base::BindRepeating(&TreeTabModel::RemoveTreeTabNode,
                                tree_tab_model_));
        auto tree_tab_node_ptr = tree_tab_node.get();

        opener_collection->AddCollection(std::move(tree_tab_node),
                                         target_index);
        CHECK_EQ(index, *collection_->GetIndexOfTabRecursive(tab_ptr))
            << "This must be match so we make sure that we inserted the tab at "
               "the specified |index|.";
        CHECK(tree_tab_model_);
        tree_tab_model_->AddTreeTabNode(tree_tab_node_ptr->node());
        return;
      }
    }
  }

  // Otherwise, we insert the new tab into the unpinned collection and then wrap
  // it with a tabs::TreeTabNodeTabCollection.
  auto* added_tab = tab.get();
  collection_->AddTabRecursive(std::move(tab), index, new_group_id,
                               new_pinned_state, GetPassKey());

  auto* unpinned_tab_collection =
      collection_->GetParentCollection(added_tab, GetPassKey());

  CHECK(unpinned_tab_collection);
  CHECK_EQ(unpinned_tab_collection->type(),
           tabs::TabCollection::Type::UNPINNED);
  auto target_index = unpinned_tab_collection->GetIndexOfTab(added_tab);
  CHECK(target_index);

  auto detached_tab = unpinned_tab_collection->MaybeRemoveTab(added_tab);
  CHECK(detached_tab);
  auto tree_tab_node = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(), std::move(detached_tab),
      base::BindRepeating(&TreeTabModel::RemoveTreeTabNode, tree_tab_model_));
  auto* tree_tab_node_ptr = tree_tab_node.get();
  unpinned_tab_collection->AddCollection(std::move(tree_tab_node),
                                         *target_index);

  CHECK(tree_tab_model_);
  tree_tab_model_->AddTreeTabNode(tree_tab_node_ptr->node());
}

std::unique_ptr<tabs::TabInterface>
BraveTreeTabStripCollectionDelegate::RemoveTabAtIndexRecursive(
    size_t index) const {
  // TODO(https://github.com/brave/brave-browser/issues/49789)
  NOTIMPLEMENTED();
  return collection_->RemoveTabAtIndexRecursive(index, GetPassKey());
}

void BraveTreeTabStripCollectionDelegate::MoveTabsRecursive(
    const std::vector<int>& tab_indices,
    size_t destination_index,
    std::optional<tab_groups::TabGroupId> new_group_id,
    bool new_pinned_state,
    const std::set<tabs::TabCollection::Type>& retain_collection_types) const {
  // TODO(https://github.com/brave/brave-browser/issues/49790)
  NOTIMPLEMENTED();
}
