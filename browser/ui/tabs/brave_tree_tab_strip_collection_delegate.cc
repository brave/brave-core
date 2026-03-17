// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/tabs/brave_tree_tab_strip_collection_delegate.h"

#include <algorithm>
#include <variant>

#include "base/containers/adapters.h"
#include "base/containers/map_util.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/notimplemented.h"
#include "brave/browser/ui/tabs/tree_tab_model.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "components/split_tabs/split_tab_visual_data.h"
#include "components/tabs/public/split_tab_collection.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_interface.h"
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
      base::BindRepeating(&TreeTabModel::RemoveTreeTabNode, tree_tab_model_),
      base::BindRepeating(&TreeTabModel::OnTreeTabNodeMoved, tree_tab_model_));
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

  // Try to add the tab to the same tree as the opener if possible.
  // Otherwise, add to unpinned collection and wrap with tree node.
  std::ignore =
      TryAddTabToSameTreeAsOpener(std::move(tab), index, opener)
          .or_else([&](auto&& tab)
                       -> BraveTreeTabStripCollectionDelegate::AddTabResult {
            AddTabToUnpinnedCollectionAsTreeNode(index, new_group_id,
                                                 std::move(tab));
            return base::ok();
          });
}

base::expected<void, std::unique_ptr<tabs::TabInterface>>
BraveTreeTabStripCollectionDelegate::TryAddTabToSameTreeAsOpener(
    std::unique_ptr<tabs::TabInterface> tab,
    size_t index,
    tabs::TabInterface* opener) const {
  if (!opener || index == 0) {
    return base::unexpected(std::move(tab));
  }

  auto* opener_collection = GetParentTreeNodeCollectionOfTab(opener);
  CHECK_EQ(opener_collection->type(), tabs::TabCollection::Type::TREE_NODE);

  tabs::TabInterface* previous_tab =
      collection_->GetTabAtIndexRecursive(index - 1);
  auto* previous_tab_collection =
      GetParentTreeNodeCollectionOfTab(previous_tab);
  CHECK_EQ(previous_tab_collection->type(),
           tabs::TabCollection::Type::TREE_NODE);

  // Check if opener and previous tab are in the same tree hierarchy.
  if (!AreInSameTreeHierarchy(opener_collection, previous_tab_collection)) {
    return base::unexpected(std::move(tab));
  }

  // Calculate target index within the opener collection.
  auto target_index = CalculateTargetIndexInOpenerCollection(
      static_cast<tabs::TreeTabNodeTabCollection*>(opener_collection), index);
  if (!target_index) {
    return base::unexpected(std::move(tab));
  }

  // Add the tab as a tree node to the opener collection.
  AddTabAsTreeNodeToCollection(std::move(tab), opener_collection, *target_index,
                               index);
  return base::ok();
}

bool BraveTreeTabStripCollectionDelegate::AreInSameTreeHierarchy(
    tabs::TabCollection* opener_collection,
    tabs::TabCollection* previous_tab_collection) const {
  CHECK_EQ(opener_collection->type(), tabs::TabCollection::Type::TREE_NODE);
  CHECK_EQ(previous_tab_collection->type(),
           tabs::TabCollection::Type::TREE_NODE);

  return static_cast<tabs::TreeTabNodeTabCollection*>(opener_collection)
             ->GetTopLevelAncestor() ==
         static_cast<tabs::TreeTabNodeTabCollection*>(previous_tab_collection)
             ->GetTopLevelAncestor();
}

std::optional<size_t>
BraveTreeTabStripCollectionDelegate::CalculateTargetIndexInOpenerCollection(
    tabs::TreeTabNodeTabCollection* opener_collection,
    size_t recursive_index) const {
  const auto& tabs = opener_collection->node().GetTabs();
  CHECK(!tabs.empty());

  const auto opener_index = *collection_->GetIndexOfTabRecursive(tabs.front());
  auto target_index = 0;
  auto tab_count = 0;

  for (auto& child :
       collection_->GetChildrenForDelegate(*opener_collection, GetPassKey())) {
    if (opener_index + tab_count == recursive_index) {
      break;
    }

    CHECK(opener_index + tab_count < recursive_index)
        << "We don't assume target index exceed the specified index.";

    target_index++;
    std::visit(absl::Overload{
                   [&](const std::unique_ptr<tabs::TabInterface>& tab) {
                     tab_count++;
                   },
                   [&](const std::unique_ptr<tabs::TabCollection>& collection) {
                     tab_count += collection->TabCountRecursive();
                   }},
               child);
  }

  // Check if specified |index| matches the calculated target index.
  // When opening an empty new tab, it is possible that they don't match.
  if (opener_index + tab_count == recursive_index) {
    return target_index;
  }

  return std::nullopt;
}

void BraveTreeTabStripCollectionDelegate::AddTabAsTreeNodeToCollection(
    std::unique_ptr<tabs::TabInterface> tab,
    tabs::TabCollection* target_collection,
    size_t target_index,
    size_t expected_recursive_index) const {
#if DCHECK_IS_ON()
  auto* tab_ptr = tab.get();
#endif
  auto tree_tab_node = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(), std::move(tab),
      base::BindRepeating(&TreeTabModel::RemoveTreeTabNode, tree_tab_model_),
      base::BindRepeating(&TreeTabModel::OnTreeTabNodeMoved, tree_tab_model_));
  auto* tree_tab_node_ptr = tree_tab_node.get();

  target_collection->AddCollection(std::move(tree_tab_node), target_index);
#if DCHECK_IS_ON()
  // Recursing index could be expensive, so we only check it when DCHECK is
  // on.
  DCHECK_EQ(expected_recursive_index,
            *collection_->GetIndexOfTabRecursive(tab_ptr))
      << "This must be match so we make sure that we inserted the tab at "
         "the specified |index|.";
#endif
  CHECK(tree_tab_model_);
  tree_tab_model_->AddTreeTabNode(tree_tab_node_ptr->node());
}

void BraveTreeTabStripCollectionDelegate::AddTabToUnpinnedCollectionAsTreeNode(
    size_t index,
    std::optional<tab_groups::TabGroupId> new_group_id,
    std::unique_ptr<tabs::TabInterface> tab) const {
  // Insert the new tab into the unpinned collection first.
  CHECK(tab);
  auto* added_tab = tab.get();
  collection_->AddTabRecursive(std::move(tab), index, new_group_id,
                               /*new_pinned_state=*/false, GetPassKey());

  // Get the unpinned collection and find the tab's index within it.
  auto* unpinned_tab_collection =
      collection_->GetParentCollection(added_tab, GetPassKey());
  CHECK(unpinned_tab_collection);
  CHECK_EQ(unpinned_tab_collection->type(),
           tabs::TabCollection::Type::UNPINNED);
  auto target_index = unpinned_tab_collection->GetIndexOfTab(added_tab);
  CHECK(target_index);

  // Remove the tab and wrap it with a tree node.
  auto detached_tab = unpinned_tab_collection->MaybeRemoveTab(added_tab);
  CHECK(detached_tab);
  auto tree_tab_node = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(), std::move(detached_tab),
      base::BindRepeating(&TreeTabModel::RemoveTreeTabNode, tree_tab_model_),
      base::BindRepeating(&TreeTabModel::OnTreeTabNodeMoved, tree_tab_model_));
  auto* tree_tab_node_ptr = tree_tab_node.get();
  unpinned_tab_collection->AddCollection(std::move(tree_tab_node),
                                         *target_index);

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

  if (parent_collection->type() != tabs::TabCollection::Type::TREE_NODE) {
    // Just remove the tab.
    return collection_->RemoveTabAtIndexRecursive(index, GetPassKey());
  }

  // When removing a tree tab node, the children node should be re-added to the
  // current position of the tree tab node.
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

void BraveTreeTabStripCollectionDelegate::MoveTabsRecursive(
    const std::vector<int>& tab_indices,
    size_t destination_index,
    std::optional<tab_groups::TabGroupId> new_group_id,
    bool new_pinned_state,
    const tabs::TabCollection::TypeEnumSet retain_collection_types) const {
  CHECK(!tab_indices.empty());
  std::vector<tabs::TabInterface*> moving_tabs;
  std::ranges::transform(
      tab_indices, std::back_inserter(moving_tabs),
      [this](int index) { return collection_->GetTabAtIndexRecursive(index); });
  CHECK(!moving_tabs.empty());

  // This can happen when put tabs together in the same position in the middle
  // of creating split or group.
  // Store the original parent collection of the first moving tab, we might need
  // this when moving to the same position so that we don't lose the tree
  // structure.
  const bool moving_to_same_position =
      destination_index == static_cast<size_t>(tab_indices.front());
  auto* first_tree_node = GetParentTreeNodeCollectionOfTab(moving_tabs.front());
  auto* original_parent_collection =
      moving_to_same_position ? first_tree_node->GetParentCollection()
                              : nullptr;
  std::optional<size_t> original_index_in_original_parent =
      original_parent_collection
          ? original_parent_collection->GetIndexOfCollection(first_tree_node)
          : std::nullopt;

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

    auto* moving_tab_parent_node = GetAttachableCollectionForTreeTabNode(
        moving_tab_tree_node->GetParentCollection());
    unique_moving_tab_collections.push_back(
        moving_tab_parent_node->MaybeRemoveCollection(moving_tab_tree_node));
  }

  if (original_parent_collection) {
    for (auto& moving_tab : base::Reversed(unique_moving_tab_collections)) {
      original_parent_collection->AddCollection(
          std::move(moving_tab), *original_index_in_original_parent);
    }
    return;
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
              if (tree_tab_node_collection->current_value_type() ==
                      tabs::TreeTabNodeTabCollection::CurrentValueType::kTab &&
                  tree_tab_node_collection->GetCurrentTab() == tab) {
                // Skipping moving current tab itself
                return;
              }

              target_collection->AddTab(
                  tree_tab_node_collection->MaybeRemoveTab(tab), target_index);
            },
            [&](tabs::TabCollection* collection) {
              if (tree_tab_node_collection->current_value_type() !=
                      tabs::TreeTabNodeTabCollection::CurrentValueType::kTab &&
                  collection ==
                      tree_tab_node_collection->GetCurrentCollection()) {
                // Skipping moving current collection itself, such as split or
                // group
                return;
              }

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
    // We don't assume that unpinned tabs are attached to unpinned collection
    // without being wrapped by a tree node.
    CHECK_NE(parent_collection->type(), tabs::TabCollection::Type::UNPINNED);
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

bool BraveTreeTabStripCollectionDelegate::CreateSplit(
    split_tabs::SplitTabId split_id,
    const std::vector<tabs::TabInterface*>& tabs,
    split_tabs::SplitTabVisualData visual_data) const {
  if (tabs.size() < 2) {
    return false;
  }

  const size_t index0 = *collection_->GetIndexOfTabRecursive(tabs[0]);
  const size_t index1 = *collection_->GetIndexOfTabRecursive(tabs[1]);
  tabs::TabInterface* first_tab = tabs[0];
  tabs::TabInterface* second_tab = tabs[1];
  if (index1 < index0) {
    std::swap(first_tab, second_tab);
  }

  // Use first tab's tree node position for insertion.
  tabs::TreeTabNodeTabCollection* first_tree_node =
      GetParentTreeNodeCollectionOfTab(first_tab);
  tabs::TabCollection* insert_parent = first_tree_node->GetParentCollection();
  size_t insert_index =
      insert_parent->GetIndexOfCollection(first_tree_node).value();
  CHECK(tree_tab_model_);

  // Remove higher recursive index first so insert_index stays valid.
  // Get recursive indices to remove higher index first (avoids index shift).
  std::vector<std::unique_ptr<tabs::TabInterface>> removed_tabs(2);
  for (tabs::TabInterface* tab : {second_tab, first_tab}) {
    tabs::TreeTabNodeTabCollection* tree_node =
        GetParentTreeNodeCollectionOfTab(tab);
    // Before removing the tree tab node from the parent, make sure all
    // children of the tree tab node are moved to the parent.
    MoveChildrenOfTreeTabNodeToParent(tree_node);

    // Clean up tree tab node from model, first so that associated tabs don't
    // reference this tree tab node anymore.
    tree_tab_model_->RemoveTreeTabNode(tree_node->node().id());

    std::unique_ptr<tabs::TabInterface> removed =
        tree_node->MaybeRemoveTab(tab);
    tabs::TabCollection* owner = tree_node->GetParentCollection();
    std::ignore = owner->MaybeRemoveCollection(tree_node);

    // Store in original order: tabs[0] at 0, tabs[1] at 1.
    const size_t orig_idx = (tab == tabs[0]) ? 0u : 1u;
    removed_tabs[orig_idx] = std::move(removed);
  }

  // Create split and add both tabs in original order.
  auto split =
      std::make_unique<tabs::SplitTabCollection>(split_id, visual_data);
  auto* removed_tab0 = removed_tabs[0].get();
  auto* removed_tab1 = removed_tabs[1].get();
  split->AddTab(std::move(removed_tabs[0]), 0);
  split->AddTab(std::move(removed_tabs[1]), 1);

  CHECK_EQ(collection_->GetParentCollection(removed_tab0, GetPassKey())->type(),
           tabs::TabCollection::Type::SPLIT);
  CHECK_EQ(collection_->GetParentCollection(removed_tab1, GetPassKey())->type(),
           tabs::TabCollection::Type::SPLIT);

  // Wrap split in a single tree node and insert at the original position.
  auto wrapper = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(), std::move(split),
      base::BindRepeating(&TreeTabModel::RemoveTreeTabNode, tree_tab_model_),
      base::BindRepeating(&TreeTabModel::OnTreeTabNodeMoved, tree_tab_model_));
  auto& node = wrapper->node();

  tabs::TabCollection::Position position = {insert_parent->GetHandle(),
                                            insert_index};
  collection_->AddTabCollectionAtPosition(std::move(wrapper), position,
                                          GetPassKey());

  // Add the tree tab node to the model and notification will be sent
  tree_tab_model_->AddTreeTabNode(node);

  CHECK_EQ(collection_->GetParentCollection(removed_tab0, GetPassKey())->type(),
           tabs::TabCollection::Type::SPLIT);
  CHECK_EQ(collection_->GetParentCollection(removed_tab1, GetPassKey())->type(),
           tabs::TabCollection::Type::SPLIT);
  return true;
}

void BraveTreeTabStripCollectionDelegate::Unsplit(
    split_tabs::SplitTabId split_id) {
  tabs::SplitTabCollection* split =
      collection_->GetSplitTabCollection(split_id);
  if (!split) {
    return;
  }

  // The split's parent is the wrapper tree node; we will destroy it too.
  tabs::TabCollection* wrapper = split->GetParentCollection();
  CHECK_EQ(wrapper->type(), tabs::TabCollection::Type::TREE_NODE);
  auto* wrapper_tree_node =
      static_cast<tabs::TreeTabNodeTabCollection*>(wrapper);
  auto* parent_collection = wrapper_tree_node->GetParentCollection();

  // Notify the model that the wrapper tree node will be removed.
  tree_tab_model_->RemoveTreeTabNode(wrapper_tree_node->node().id());

  // 1. Extract tabs in split and move them to the wrapper's parent at the same
  // position of the wrapper, wrapped with tree nodes.

  std::vector<tabs::TabInterface*> tabs = split->GetTabsRecursive();
  CHECK(!tabs.empty());
  size_t target_index =
      parent_collection->GetIndexOfCollection(wrapper).value();
  size_t expected_recursive_index =
      collection_->GetIndexOfTabRecursive(tabs[0]).value();
  for (size_t i = 0; i < tabs.size(); ++i) {
    std::unique_ptr<tabs::TabInterface> detached =
        split->MaybeRemoveTab(tabs[i]);
    CHECK(detached);
    AddTabAsTreeNodeToCollection(std::move(detached), parent_collection,
                                 target_index++, expected_recursive_index++);
  }

  // 2. Move children of the wrapper tree node to the parent collection at the
  // same position of the wrapper.
  MoveChildrenOfTreeTabNodeToNode(wrapper_tree_node, parent_collection,
                                  target_index);

  // 3. Remove the its wrapper(and its owned split collection)
  collection_->RemoveTabCollection(wrapper);
}

tabs::TabCollection*
BraveTreeTabStripCollectionDelegate::GetCollectionForMapping(
    tabs::TabCollection* root_collection) {
  if (root_collection->type() == tabs::TabCollection::Type::TREE_NODE) {
    auto* tree_tab_node_collection =
        static_cast<tabs::TreeTabNodeTabCollection*>(root_collection);
    if (tree_tab_node_collection->current_value_type() !=
        tabs::TreeTabNodeTabCollection::CurrentValueType::kTab) {
      auto* collection = tree_tab_node_collection->GetCurrentCollection();
      CHECK(collection);
      return collection;
    }
  }

  return root_collection;
}
