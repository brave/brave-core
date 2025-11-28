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
      // Find insertion index among the opener collection
      const auto opener_index = *collection_->GetIndexOfTabRecursive(
          static_cast<tabs::TreeTabNodeTabCollection*>(opener_collection)
              ->current_tab()
              .get());
      auto target_index = 0;
      auto tab_count = 0;

      for (auto& child :
           collection_->GetChildrenForDelegate(*opener_collection, GetPassKey())) {
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
  CHECK_EQ(parent_collection->type(), tabs::TabCollection::Type::UNPINNED);
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
    CHECK(tree_tab_model_);
    tree_tab_model_->RemoveTreeTabNode(tree_tab_node_collection->node().id());

    // Remove the tab first so that we can keep the |index| correct.
    auto tab = collection_->RemoveTabAtIndexRecursive(index, GetPassKey());

    // Get direct children of |tree_tab_node_collection| and re-add them to the
    // owner collection to preserve the tree structure.
    MoveChildrenOfTreeTabNodeToParent(tree_tab_node_collection);

    // Remove the tree tab node collection from owner
    auto* tree_node_owner_collection =
        tree_tab_node_collection->GetParentCollection();
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
  std::vector<tabs::TabInterface*> moving_tabs;
  std::ranges::transform(
      tab_indices, std::back_inserter(moving_tabs),
      [this](int index) { return collection_->GetTabAtIndexRecursive(index); });
  CHECK(!moving_tabs.empty());

  // Before removing the tree tab node from the parent, make sure all
  // children of the tree tab node are moved to the parent.
  for (auto* moving_tab : base::Reversed(moving_tabs)) {
    auto* moving_tab_tree_node = GetParentTreeNodeCollectionOfTab(moving_tab);
    MoveChildrenOfTreeTabNodeToParent(
        static_cast<tabs::TreeTabNodeTabCollection*>(moving_tab_tree_node));
  }

  // Remove the moving tab first from the original parent.
  std::vector<std::unique_ptr<tabs::TabCollection>>
      unique_moving_tab_collections;
  for (auto* moving_tab : base::Reversed(moving_tabs)) {
    auto* moving_tab_tree_node = GetParentTreeNodeCollectionOfTab(moving_tab);
    CHECK(moving_tab_tree_node);

    auto* moving_tab_parent_node =
        GetAttachableCollectionForTreeTabNode(moving_tab_tree_node->GetParentCollection());
    unique_moving_tab_collections.push_back(
        moving_tab_parent_node->MaybeRemoveCollection(moving_tab_tree_node));
  }

  // We'll move the moving tab's tree node into the collection of the
  // destination index.
  const auto tab_count = collection_->TabCountRecursive();
  const bool insert_after = destination_index >= tab_count;
  auto* destination_tab = collection_->GetTabAtIndexRecursive(
      destination_index >= tab_count ? tab_count - 1 : destination_index);
  auto* destination_tree_node =
      GetParentTreeNodeCollectionOfTab(destination_tab);

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

  // Insert the moving tab collection into the new parent collection.
  auto index = new_parent_node_for_moving_tab->GetIndexOfCollection(
      destination_tree_node);
  // TODO() When group/split is involved, this check will fail.
  CHECK(index.has_value());
  index = insert_after ? *index + 1 : *index;
  for (auto& unique_moving_tab_collection : unique_moving_tab_collections) {
    new_parent_node_for_moving_tab->AddCollection(
        std::move(unique_moving_tab_collection), *index);
  }
}

void BraveTreeTabStripCollectionDelegate::MoveChildrenOfTreeTabNodeToParent(
    tabs::TreeTabNodeTabCollection* tree_tab_node_collection) const {
  auto* tree_node_owner_collection =
      tree_tab_node_collection->GetParentCollection();
  auto local_index = tree_node_owner_collection->GetIndexOfCollection(
      tree_tab_node_collection);
  CHECK(local_index.has_value());

  MoveChildrenOfTreeTabNodeToNode(tree_tab_node_collection,
                                  tree_node_owner_collection, *local_index);
}

void BraveTreeTabStripCollectionDelegate::MoveChildrenOfTreeTabNodeToNode(
    tabs::TreeTabNodeTabCollection* tree_tab_node_collection,
    tabs::TabCollection* target_collection,
    size_t target_index) const {
  CHECK_LT(target_index, target_collection->ChildCount());

  auto children = tree_tab_node_collection->GetTreeNodeChildren();
  for (auto& child : base::Reversed(children)) {
    std::visit(
        absl::Overload{
            [&](tabs::TabInterface* tab) {
              if (tab == tree_tab_node_collection->current_tab().get()) {
                // Skipping moving current tab itself
                return;
              }

              target_collection->AddTab(
                  tree_tab_node_collection->MaybeRemoveTab(tab), target_index);
            },
            [&](tabs::TabCollection* collection) {
              target_collection->AddCollection(
                  tree_tab_node_collection->MaybeRemoveCollection(collection),
                  target_index);
            }},
        child);
  }
}

tabs::TreeTabNodeTabCollection*
BraveTreeTabStripCollectionDelegate::GetParentTreeNodeCollectionOfTab(
    tabs::TabInterface* tab) const {
  auto* parent_collection = collection_->GetParentCollection(tab, GetPassKey());
  CHECK(parent_collection);
  while (parent_collection->type() != tabs::TabCollection::Type::TREE_NODE) {
    parent_collection = parent_collection->GetParentCollection();
    CHECK(parent_collection);
    if (parent_collection->type() == tabs::TabCollection::Type::UNPINNED) {
      return nullptr;
    }
  }
  return static_cast<tabs::TreeTabNodeTabCollection*>(parent_collection);
}

tabs::TabCollection*
BraveTreeTabStripCollectionDelegate::GetAttachableCollectionForTreeTabNode(
    tabs::TabCollection* attachable_collection) const {
  CHECK(attachable_collection);
  while (attachable_collection->type() !=
             tabs::TabCollection::Type::TREE_NODE &&
         attachable_collection->type() != tabs::TabCollection::Type::UNPINNED) {
    attachable_collection = attachable_collection->GetParentCollection();
    CHECK(attachable_collection);
  }
  return attachable_collection;
}
