// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/tabs/brave_tree_tab_strip_collection_delegate.h"

#include <variant>

#include "base/containers/adapters.h"
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

  auto* opener_collection =
      collection_->GetParentCollection(opener, GetPassKey());
  CHECK_EQ(opener_collection->type(), tabs::TabCollection::Type::TREE_NODE);

  tabs::TabInterface* previous_tab =
      collection_->GetTabAtIndexRecursive(index - 1);
  auto* previous_tab_collection =
      collection_->GetParentCollection(previous_tab, GetPassKey());
  CHECK_EQ(previous_tab_collection->type(),
           tabs::TabCollection::Type::TREE_NODE);

  // Check if opener and previous tab are in the same tree hierarchy.
  if (!AreInSameTreeHierarchy(opener_collection, previous_tab_collection)) {
    return base::unexpected(std::move(tab));
  }

  // Calculate target index within the opener collection.
  auto target_index =
      CalculateTargetIndexInOpenerCollection(opener_collection, index);
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
    tabs::TabCollection* opener_collection,
    size_t recursive_index) const {
  const auto opener_index = *collection_->GetIndexOfTabRecursive(
      static_cast<tabs::TreeTabNodeTabCollection*>(opener_collection)
          ->current_tab()
          .get());
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
      base::BindRepeating(&TreeTabModel::RemoveTreeTabNode, tree_tab_model_));
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
    const std::set<tabs::TabCollection::Type>& retain_collection_types) const {
  // TODO(https://github.com/brave/brave-browser/issues/49790)
  NOTIMPLEMENTED();
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
