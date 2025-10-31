// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/tabs/brave_tree_tab_strip_collection_delegate.h"

#include <variant>

#include "base/containers/adapters.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
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
      // Find insertion index among the opener collection
      const auto opener_index = *collection_->GetIndexOfTabRecursive(
          static_cast<tabs::TreeTabNodeTabCollection*>(opener_collection)
              ->current_tab());
      auto target_index = 0;
      auto tab_count = 0;

      for (auto& child :
           collection_->GetChildren(*opener_collection, GetPassKey())) {
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
      // In that case, we just add the new tab to the current collection below.
      if (opener_index + tab_count == index) {
        auto* tab_ptr = tab.get();
        auto tree_tab_node = std::make_unique<tabs::TreeTabNodeTabCollection>(
            tree_tab::TreeTabNodeId::GenerateNew(), std::move(tab));
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
  auto* tree_tab_node_ptr = tree_tab_node.get();
  parent_collection->AddCollection(std::move(tree_tab_node), *target_index);

  CHECK(tree_tab_model_);
  tree_tab_model_->AddTreeTabNode(tree_tab_node_ptr->node());
}

std::unique_ptr<tabs::TabInterface>
BraveTreeTabStripCollectionDelegate::RemoveTabAtIndexRecursive(
    size_t index) const {
  auto* target_tab = collection_->GetTabAtIndexRecursive(index);
  auto* parent_collection =
      collection_->GetParentCollection(target_tab, GetPassKey());
  CHECK(parent_collection);

  if (parent_collection->type() == tabs::TabCollection::Type::TREE_NODE) {
    auto* tree_tab_node_collection =
        static_cast<tabs::TreeTabNodeTabCollection*>(parent_collection);

    auto* tree_node_owner_collection =
        tree_tab_node_collection->GetParentCollection();

    // Remove the tab first so that we can keep the |index| correct.
    auto tab = collection_->RemoveTabAtIndexRecursive(index, GetPassKey());

    // Get direct children of |tree_tab_node_collection| and re-add them to the
    // owner collection to preserve the tree structure.
    auto local_index = tree_node_owner_collection->GetIndexOfCollection(
        tree_tab_node_collection);
    CHECK(local_index.has_value());

    auto children = tree_tab_node_collection->GetTreeNodeChildren();
    for (auto& child : base::Reversed(children)) {
      std::visit(
          absl::Overload{
              [&](tabs::TabInterface* tab) {
                tree_node_owner_collection->AddTab(
                    tree_tab_node_collection->MaybeRemoveTab(tab),
                    *local_index);
              },
              [&](tabs::TabCollection* collection) {
                tree_node_owner_collection->AddCollection(
                    tree_tab_node_collection->MaybeRemoveCollection(collection),
                    *local_index);
              }},
          child);
    }

    // Remove the tree tab node collection form owner
    CHECK(tree_tab_model_);
    tree_tab_model_->RemoveTreeTabNode(tree_tab_node_collection->node().id());

    CHECK(tree_node_owner_collection);
    auto removed_collection = tree_node_owner_collection->MaybeRemoveCollection(
        tree_tab_node_collection);
    return tab;
  }

  // Just remove the tab.
  return collection_->RemoveTabAtIndexRecursive(index, GetPassKey());
}

void BraveTreeTabStripCollectionDelegate::MoveTabsRecursive(
    const std::vector<int>& tab_indices,
    size_t destination_index,
    std::optional<tab_groups::TabGroupId> new_group_id,
    bool new_pinned_state,
    const std::set<tabs::TabCollection::Type>& retain_collection_types) const {
  CHECK(!tab_indices.empty());
  LOG(ERROR) << __FUNCTION__ << " moving tab from " << tab_indices[0] << " to "
             << destination_index;

  // Find the nearest tree tab node collection of the first tab.
  auto* moving_tab = collection_->GetTabAtIndexRecursive(tab_indices[0]);
  CHECK(moving_tab);

  auto find_nearest_parent_tree_node = [&](tabs::TabInterface* tab) {
    auto* parent_collection =
        collection_->GetParentCollection(tab, GetPassKey());
    CHECK(parent_collection);
    while (parent_collection->type() != tabs::TabCollection::Type::TREE_NODE) {
      parent_collection = parent_collection->GetParentCollection();
      CHECK(parent_collection);
    }
    return parent_collection;
  };
  auto* moving_tab_tree_node = find_nearest_parent_tree_node(moving_tab);

  // Remove the moving tab first from the original parent.
  auto* moving_tab_parent_node = moving_tab_tree_node->GetParentCollection();
  while (
      moving_tab_parent_node->type() != tabs::TabCollection::Type::TREE_NODE &&
      moving_tab_parent_node->type() != tabs::TabCollection::Type::UNPINNED) {
    moving_tab_parent_node = moving_tab_parent_node->GetParentCollection();
    CHECK(moving_tab_parent_node);
  };

  CHECK(moving_tab_parent_node->ContainsCollection(moving_tab_tree_node));
  auto unique_moving_tab_collection =
      moving_tab_parent_node->MaybeRemoveCollection(moving_tab_tree_node);

  // ----------------

  // We'll move the parent collection into the collection of the destination
  // index.
  const auto tab_count = collection_->TabCountRecursive();
  auto* destination_tab = collection_->GetTabAtIndexRecursive(
      destination_index >= tab_count ? tab_count - 1 : destination_index);

  auto* destination_tree_node = find_nearest_parent_tree_node(destination_tab);
  // LOG(ERROR) << "destination tab index: "
  //            << *collection_->GetIndexOfTabRecursive(destination_tab);

  // Insert the parent tree node collection to the destination parent collection
  // so that the parent tree node could be located before the destination parent
  // collection.
  // TODO(sko) Handle the case where we'd like to put the tabs "under the
  // destination"
  auto* new_parent_node_for_moving_tab =
      destination_tree_node->GetParentCollection();
  CHECK(new_parent_node_for_moving_tab);
  while (new_parent_node_for_moving_tab->type() !=
             tabs::TabCollection::Type::TREE_NODE &&
         new_parent_node_for_moving_tab->type() !=
             tabs::TabCollection::Type::UNPINNED) {
    new_parent_node_for_moving_tab =
        new_parent_node_for_moving_tab->GetParentCollection();
    CHECK(new_parent_node_for_moving_tab);
  }
  if (new_parent_node_for_moving_tab->type() ==
      tabs::TabCollection::Type::UNPINNED) {
    LOG(ERROR) << "new parent node for moving tab found: "
                  "Unpinned ";
  } else if (new_parent_node_for_moving_tab->type() ==
             tabs::TabCollection::Type::TREE_NODE) {
    auto index = collection_->GetIndexOfTabRecursive(
        static_cast<tabs::TreeTabNodeTabCollection*>(
            new_parent_node_for_moving_tab)
            ->current_tab());
    CHECK(index.has_value());
    LOG(ERROR) << "new parent node for moving tab found: "
               << base::NumberToString(*index);
  } else {
    NOTREACHED();
  }

  // Insert the moving tab collection into the new parent collection.
  auto index = new_parent_node_for_moving_tab->GetIndexOfCollection(
      destination_tree_node);
  // TODO() When group/split is involved, this check will fail.
  CHECK(index.has_value());
  new_parent_node_for_moving_tab->AddCollection(
      std::move(unique_moving_tab_collection), *index);
  LOG(ERROR) << "Result  among tabstrip: "
             << *collection_->GetIndexOfTabRecursive(moving_tab);
}
