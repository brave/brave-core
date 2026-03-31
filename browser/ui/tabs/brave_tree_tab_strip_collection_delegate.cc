// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/tabs/brave_tree_tab_strip_collection_delegate.h"

#include <algorithm>
#include <variant>

#include "base/auto_reset.h"
#include "base/containers/adapters.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/notimplemented.h"
#include "base/notreached.h"
#include "brave/browser/ui/tabs/tree_tab_model.h"
#include "brave/components/tabs/public/brave_tab_strip_collection.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "components/split_tabs/split_tab_visual_data.h"
#include "components/tabs/public/pinned_tab_collection.h"
#include "components/tabs/public/split_tab_collection.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_group_tab_collection.h"
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

  if (new_group_id.has_value()) {
    // In this case, the tab is being added to a group. We don't need to wrap it
    // with a tree node as the group will wrap it.
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

void BraveTreeTabStripCollectionDelegate::InsertTabCollectionAt(
    std::unique_ptr<tabs::TabCollection> collection,
    int index,
    bool pinned,
    std::optional<tab_groups::TabGroupId> parent_group) {
  // This method is called when moving a entire collection from another window
  // into this window. In this case, we should check if we should wrap the
  // group or split collection with a tree node.
  if (pinned || parent_group.has_value()) {
    // * If a collection is pinned, we don't support tree mode.
    // * if a collection is in a group, the collection should be a direct child
    // of the group.
    CHECK_EQ(collection->type(), tabs::TabCollection::Type::SPLIT);
    collection_->InsertTabCollectionAt(std::move(collection), index, pinned,
                                       parent_group, GetPassKey());
    return;
  }

  if (collection->type() == tabs::TabCollection::Type::SPLIT) {
    auto split_unique = base::WrapUnique<tabs::SplitTabCollection>(
        static_cast<tabs::SplitTabCollection*>(collection.release()));
    WrapCollectionInTreeNodeAndInsert(std::move(split_unique), index, pinned,
                                      parent_group);
  } else if (collection->type() == tabs::TabCollection::Type::GROUP) {
    auto group_unique = base::WrapUnique<tabs::TabGroupTabCollection>(
        static_cast<tabs::TabGroupTabCollection*>(collection.release()));
    WrapCollectionInTreeNodeAndInsert(std::move(group_unique), index, pinned,
                                      parent_group);
  } else {
    NOTREACHED();
  }
}

void BraveTreeTabStripCollectionDelegate::InsertTreeNodeWrapperAt(
    std::unique_ptr<tabs::TreeTabNodeTabCollection> wrapper,
    int index,
    bool pinned,
    std::optional<tab_groups::TabGroupId> parent_group) {
  CHECK(tree_tab_model_);
  tabs::TreeTabNodeTabCollection* tree_node = wrapper.get();
  collection_->InsertTabCollectionAt(std::move(wrapper), index, pinned,
                                     parent_group, GetPassKey());
  tree_tab_model_->AddTreeTabNode(tree_node->node());
}

void BraveTreeTabStripCollectionDelegate::WrapCollectionInTreeNodeAndInsert(
    std::unique_ptr<tabs::SplitTabCollection> collection,
    int index,
    bool pinned,
    std::optional<tab_groups::TabGroupId> parent_group) {
  auto wrapper = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(), std::move(collection),
      base::BindRepeating(&TreeTabModel::RemoveTreeTabNode, tree_tab_model_),
      base::BindRepeating(&TreeTabModel::OnTreeTabNodeMoved, tree_tab_model_));
  InsertTreeNodeWrapperAt(std::move(wrapper), index, pinned, parent_group);
}

void BraveTreeTabStripCollectionDelegate::WrapCollectionInTreeNodeAndInsert(
    std::unique_ptr<tabs::TabGroupTabCollection> collection,
    int index,
    bool pinned,
    std::optional<tab_groups::TabGroupId> parent_group) {
  auto wrapper = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(), std::move(collection),
      base::BindRepeating(&TreeTabModel::RemoveTreeTabNode, tree_tab_model_),
      base::BindRepeating(&TreeTabModel::OnTreeTabNodeMoved, tree_tab_model_));
  InsertTreeNodeWrapperAt(std::move(wrapper), index, pinned, parent_group);
}

std::unique_ptr<tabs::SplitTabCollection>
BraveTreeTabStripCollectionDelegate::UnwrapTreeNodeForCollection(
    tabs::SplitTabCollection* collection) {
  CHECK(collection);
  CHECK(collection->GetParentCollection() &&
        collection->GetParentCollection()->type() ==
            tabs::TabCollection::Type::TREE_NODE);

  auto* wrapper = static_cast<tabs::TreeTabNodeTabCollection*>(
      collection->GetParentCollection());

  CHECK(tree_tab_model_);
  tree_tab_model_->RemoveTreeTabNode(wrapper->node().id());

  MoveChildrenOfTreeTabNodeToParent(wrapper);

  auto owned_split_collection =
      base::WrapUnique(static_cast<tabs::SplitTabCollection*>(
          wrapper->MaybeRemoveCollection(collection).release()));

  auto* owner = wrapper->GetParentCollection();
  CHECK(owner);
  std::ignore = owner->MaybeRemoveCollection(wrapper);

  return owned_split_collection;
}

tabs::TabCollection*
BraveTreeTabStripCollectionDelegate::GetParentCollectionSkippingTypes(
    tabs::TabInterface* tab,
    const tabs::TabCollection::TypeEnumSet& skipping_types) const {
  tabs::TabCollection* parent =
      collection_->GetParentCollection(tab, GetPassKey());
  if (skipping_types.Has(parent->type())) {
    parent = parent->GetParentCollection();
  }
  return parent;
}

base::flat_set<tab_groups::TabGroupId>
BraveTreeTabStripCollectionDelegate::GetMovingGroups(
    const std::vector<tabs::TabInterface*>& moving_tabs,
    std::optional<tab_groups::TabGroupId> new_group_id,
    bool new_pinned_state,
    const tabs::TabCollection::TypeEnumSet& retain_collection_types) const {
  if (new_group_id || new_pinned_state ||
      !retain_collection_types.Has(tabs::TabCollection::Type::GROUP)) {
    return {};
  }

  base::flat_map<tab_groups::TabGroupId, std::vector<tabs::TabInterface*>>
      moving_tabs_by_group;
  for (auto* tab : moving_tabs) {
    if (auto group = tab->GetGroup()) {
      moving_tabs_by_group[group.value()].push_back(tab);
    }
  }

  // Checks that every tab in the group is moving together.
  base::flat_set<tab_groups::TabGroupId> moving_groups;
  for (const auto& [group_id, group_tabs] : moving_tabs_by_group) {
    auto* group_collection = collection_->GetTabGroupCollection(group_id);
    if (!group_collection ||
        group_collection->TabCountRecursive() != group_tabs.size()) {
      continue;
    }

    if (std::ranges::all_of(group_tabs, [group_collection](auto* tab) {
          return group_collection->GetIndexOfTabRecursive(tab).has_value();
        })) {
      moving_groups.insert(group_id);
    }
  }
  return moving_groups;
}

std::vector<std::variant<tabs::TabInterface*, tabs::TabCollection*>>
BraveTreeTabStripCollectionDelegate::CompactMovingTabs(
    const std::vector<tabs::TabInterface*>& moving_tabs,
    const tabs::TabCollection::TypeEnumSet& types_to_compact) const {
  std::vector<std::variant<tabs::TabInterface*, tabs::TabCollection*>>
      compacted_tabs;

  base::flat_set<tab_groups::TabGroupId> moving_groups;
  if (types_to_compact.Has(tabs::TabCollection::Type::GROUP)) {
    for (auto* tab : moving_tabs) {
      if (auto group = tab->GetGroup()) {
        moving_groups.insert(group.value());
      }
    }
  }

  base::flat_set<split_tabs::SplitTabId> moving_splits;
  if (types_to_compact.Has(tabs::TabCollection::Type::SPLIT)) {
    for (auto* tab : moving_tabs) {
      if (tab->IsSplit()) {
        moving_splits.insert(tab->GetSplit().value());
      }
    }
  }

  base::flat_set<tab_groups::TabGroupId> compacted_groups;
  base::flat_set<split_tabs::SplitTabId> compacted_splits;
  for (auto* tab : moving_tabs) {
    if (auto group = tab->GetGroup();
        group && moving_groups.contains(group.value())) {
      if (compacted_groups.contains(group.value())) {
        // The entire group is already added.
        continue;
      }

      compacted_groups.insert(group.value());
      auto* group_collection = GetParentCollectionSkippingTypes(
          tab, {tabs::TabCollection::Type::SPLIT});
      CHECK_EQ(group_collection->type(), tabs::TabCollection::Type::GROUP);
      compacted_tabs.push_back(group_collection);
      continue;
    }

    if (tab->IsSplit() && moving_splits.contains(tab->GetSplit().value())) {
      if (compacted_splits.contains(tab->GetSplit().value())) {
        // The entire split is already added.
        continue;
      }

      compacted_splits.insert(tab->GetSplit().value());
      compacted_tabs.push_back(
          collection_->GetParentCollection(tab, GetPassKey()));
      continue;
    }

    compacted_tabs.push_back(std::move(tab));
  }

  return compacted_tabs;
}

base::expected<void, std::unique_ptr<tabs::TabInterface>>
BraveTreeTabStripCollectionDelegate::TryAddTabToSameTreeAsOpener(
    std::unique_ptr<tabs::TabInterface> tab,
    size_t index,
    tabs::TabInterface* opener) const {
  // If new tab is inserted at first or last without opener, it becomes child
  // node of unpinned collection.
  if (index == collection_->IndexOfFirstNonPinnedTab() ||
      (!opener && index == collection_->TabCountRecursive())) {
    return base::unexpected(std::move(tab));
  }

  if (opener && IsTabInPinnedCollection(opener)) {
    // Unpinned tab is being created from a pinned tab. We can't add the new
    // tab to the same tree as the opener.
    return base::unexpected(std::move(tab));
  }

  tabs::TreeTabNodeTabCollection* opener_collection = nullptr;
  if (opener) {
    opener_collection = static_cast<tabs::TreeTabNodeTabCollection*>(
        GetParentTreeNodeCollectionOfTab(opener));
  } else {
    // in case of opening a new tab in the midle of tabs, we should try to add
    // the new tab to the same tree. This can happen when opening a new tab via
    // "New Tab below" item in the context menu.
    auto* tree_collection = GetParentTreeNodeCollectionOfTab(
        collection_->GetTabAtIndexRecursive(index));
    CHECK(tree_collection->type() == tabs::TabCollection::Type::TREE_NODE);

    // Find parent tree node collection until we reach the unpinned collection.
    auto* parent_collection = GetAttachableCollectionForTreeTabNode(
        tree_collection->GetParentCollection());
    CHECK(parent_collection);

    if (parent_collection->type() == tabs::TabCollection::Type::UNPINNED) {
      // If the |tree_collection| is attached to the unpinned collection, we
      // don't proceed to add the tab to the tree.
      return base::unexpected(std::move(tab));
    }

    opener_collection =
        static_cast<tabs::TreeTabNodeTabCollection*>(parent_collection);
  }

  CHECK(opener_collection);
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
  CHECK(!new_group_id.has_value());
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
    const tabs::TabCollection::TypeEnumSet retain_collection_types) {
  // TabStripModel::AddToExistingGroupImpl only moves tabs strictly before or
  // after the group’s first/last strip index; indices already in [first, last]
  // are dropped. If none remain, this runs with an empty tab_indices (e.g.
  // no-op add-to-group).
  if (tab_indices.empty()) {
    return;
  }

  std::vector<tabs::TabInterface*> moving_tabs;
  std::ranges::transform(
      tab_indices, std::back_inserter(moving_tabs),
      [this](int index) { return collection_->GetTabAtIndexRecursive(index); });
  CHECK(!moving_tabs.empty());

  const bool pinned_state_inconsistent =
      new_pinned_state != std::ranges::any_of(moving_tabs, [this](auto* tab) {
        return IsTabInPinnedCollection(tab);
      });
  if (pinned_state_inconsistent && new_group_id) {
    CHECK(!new_pinned_state);
    CHECK(!handling_pinned_state_inconsistency_);

    base::AutoReset<bool> auto_reset(&handling_pinned_state_inconsistency_,
                                     true);

    // In this case, we should unpin and move tabs first in order to unpin the
    // tabs, then add to group.
    UnpinTabs(moving_tabs, destination_index, new_group_id,
              retain_collection_types);
    return;
  }

  // Move out of group: tabs are in a TabGroupTabCollection. Only when not
  // moving into another group (no new_group_id). Note that the indices could
  // be from multiple groups or including non-grouped tabs.
  const base::flat_set<tab_groups::TabGroupId> moving_groups = GetMovingGroups(
      moving_tabs, new_group_id, new_pinned_state, retain_collection_types);
  const bool should_ungroup =
      !new_group_id.has_value() &&
      std::ranges::any_of(moving_tabs, [&, this](auto* tab) {
        auto* non_split_parent = GetParentCollectionSkippingTypes(
            tab, {tabs::TabCollection::Type::SPLIT});
        if (non_split_parent->type() != tabs::TabCollection::Type::GROUP) {
          // Already not in a group
          return false;
        }

        // In case the tab is in a moving group, we should not ungroup it.
        auto group_id =
            static_cast<tabs::TabGroupTabCollection*>(non_split_parent)
                ->GetTabGroupId();
        if (tab->GetGroup() && tab->GetGroup() != group_id) {
          // Tab should be moved out of group because it's belong to wrong group
          // collection
          return true;
        }

        return !moving_groups.contains(group_id);
      });
  if (should_ungroup) {
    MoveTabsOutOfGroup(moving_tabs, moving_groups, destination_index,
                       new_pinned_state, retain_collection_types);
    return;
  }

  // Move into group: unwrap tabs from tree nodes and add to the group.
  if (new_group_id.has_value()) {
    CHECK(!new_pinned_state);
    MoveTabsIntoGroup(moving_tabs, destination_index, *new_group_id,
                      retain_collection_types);
    return;
  }

  if (new_pinned_state) {
    PinTabs(moving_tabs, destination_index);
    return;
  }

  if (pinned_state_inconsistent) {
    CHECK(!handling_pinned_state_inconsistency_);
    base::AutoReset<bool> auto_reset(&handling_pinned_state_inconsistency_,
                                     true);
    UnpinTabs(moving_tabs, destination_index, new_group_id,
              retain_collection_types);
    return;
  }

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
  auto compacted_moving_tabs = CompactMovingTabs(
      moving_tabs,
      {tabs::TabCollection::Type::GROUP, tabs::TabCollection::Type::SPLIT});
  std::vector<std::unique_ptr<tabs::TabCollection>>
      unique_moving_tab_collections_reversed;
  for (auto& tab_or_collection : base::Reversed(compacted_moving_tabs)) {
    tabs::TabCollection* moving_tab_tree_node = std::visit(
        absl::Overload(
            [this](tabs::TabInterface* tab) {
              return collection_->GetParentCollection(tab, GetPassKey());
            },
            [](tabs::TabCollection* collection) {
              return collection->GetParentCollection();
            }),
        tab_or_collection);
    CHECK(moving_tab_tree_node);
    CHECK_EQ(moving_tab_tree_node->type(),
             tabs::TabCollection::Type::TREE_NODE);
    auto* moving_tab_parent_node = GetAttachableCollectionForTreeTabNode(
        moving_tab_tree_node->GetParentCollection());
    unique_moving_tab_collections_reversed.push_back(
        moving_tab_parent_node->MaybeRemoveCollection(moving_tab_tree_node));
  }

  // Insert to the target position and collection.
  if (original_parent_collection) {
    // Moving to the same position in the same collection.
    for (auto& moving_tab : unique_moving_tab_collections_reversed) {
      original_parent_collection->AddCollection(
          std::move(moving_tab), *original_index_in_original_parent);
    }
    return;
  }

  // In case of moving groups, we should not use destination index if a tab at
  // the index is in a group. We should find the fist tab or last index of the
  // group instead.
  const auto tab_count = collection_->TabCountRecursive();
  auto* destination_tab = collection_->GetTabAtIndexRecursive(
      destination_index >= tab_count ? tab_count - 1 : destination_index);
  if (!moving_groups.empty() && destination_tab->GetGroup()) {
    auto group_id = *destination_tab->GetGroup();
    auto* group_collection = collection_->GetTabGroupCollection(group_id);
    CHECK(group_collection);
    auto group_tab_count = group_collection->TabCountRecursive();
    CHECK(group_tab_count);
    auto first_tab = *collection_->GetIndexOfTabRecursive(
        group_collection->GetTabAtIndexRecursive(0));
    auto last_tab = *collection_->GetIndexOfTabRecursive(
        group_collection->GetTabAtIndexRecursive(group_tab_count - 1));

    destination_index = (first_tab + last_tab) / 2 < destination_index
                            ? last_tab + 1
                            : first_tab;
  }

  // We'll move the moving tab's tree node into the collection of the
  // destination index.
  const bool insert_after = destination_index >= tab_count;
  auto* destination_tree_node =
      GetParentTreeNodeCollectionOfTab(destination_tab);

  // Insert the parent tree node collection to the destination parent collection
  // so that the parent tree node could be located before the destination parent
  // collection.
  auto* new_parent_node_for_moving_tab =
      destination_tree_node->GetParentCollection();
  CHECK(new_parent_node_for_moving_tab);
  // TreeTabNodeTabCollection must be child of another node or unpinned
  // collection.
  CHECK(new_parent_node_for_moving_tab->type() ==
            tabs::TabCollection::Type::TREE_NODE ||
        new_parent_node_for_moving_tab->type() ==
            tabs::TabCollection::Type::UNPINNED);

  // Insert the moving tab collection into the new parent collection.
  auto index = new_parent_node_for_moving_tab->GetIndexOfCollection(
      destination_tree_node);
  CHECK(index.has_value());
  index = insert_after ? *index + 1 : *index;
  for (auto& unique_moving_tab_collection :
       unique_moving_tab_collections_reversed) {
    new_parent_node_for_moving_tab->AddCollection(
        std::move(unique_moving_tab_collection), *index);
  }
}

void BraveTreeTabStripCollectionDelegate::AttachDetachedGroupCollection(
    const std::vector<tabs::TabInterface*>& moving_tabs,
    tab_groups::TabGroupId new_group_id) const {
  // Group is detached state, this can happen when creating a new group;
  // wrap it in a tree node and attach at the position
  // where the first tab being moved into the group currently lives (same
  // parent and index), so the new group is nested correctly in the tree.
  // The tab may be in a TREE_NODE (standalone) or in a GROUP (then use that
  // group's wrapper tree node).
  tabs::TabCollection* tree_node_for_position = nullptr;
  for (tabs::TabInterface* tab : moving_tabs) {
    // Note that we should skip GROUP here too because the the tab is moving
    // from a group to another new group.
    auto* parent = GetParentCollectionSkippingTypes(
        tab,
        {tabs::TabCollection::Type::SPLIT, tabs::TabCollection::Type::GROUP});
    if (parent->type() == tabs::TabCollection::Type::TREE_NODE) {
      tree_node_for_position = parent;
      break;
    }
  }
  CHECK(tree_node_for_position)
      << "At least one moving tab must be in a tree node or in a group";

  tabs::TabCollection* parent = GetAttachableCollectionForTreeTabNode(
      tree_node_for_position->GetParentCollection());
  std::optional<size_t> index_in_parent =
      parent->GetIndexOfCollection(tree_node_for_position);
  CHECK(index_in_parent.has_value());

  tabs::TabCollection::Position position{parent->GetHandle(), *index_in_parent};

  std::unique_ptr<tabs::TabGroupTabCollection> group =
      collection_->PopDetachedGroupCollectionForDelegate(new_group_id,
                                                         GetPassKey());
  CHECK(group);
  auto wrapper = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(), std::move(group),
      base::BindRepeating(&TreeTabModel::RemoveTreeTabNode, tree_tab_model_),
      base::BindRepeating(&TreeTabModel::OnTreeTabNodeMoved, tree_tab_model_));
  tabs::TreeTabNode& node = wrapper->node();
  collection_->AddTabCollectionAtPosition(std::move(wrapper), position,
                                          GetPassKey());
  CHECK(tree_tab_model_);
  tree_tab_model_->AddTreeTabNode(node);
}

void BraveTreeTabStripCollectionDelegate::MoveTabsIntoGroup(
    const std::vector<tabs::TabInterface*>& moving_tabs,
    size_t destination_index,
    tab_groups::TabGroupId new_group_id,
    const tabs::TabCollection::TypeEnumSet& retain_collection_types) {
  tabs::TabGroupTabCollection* group_collection =
      collection_->GetTabGroupCollection(new_group_id);

  if (!group_collection) {
    // In this case, group is created but in detached state yet. We need to
    // attach it to the collection.
    AttachDetachedGroupCollection(moving_tabs, new_group_id);
    group_collection = collection_->GetTabGroupCollection(new_group_id);
    CHECK(group_collection);
  }

  auto compacted_moving_tabs =
      CompactMovingTabs(moving_tabs, {tabs::TabCollection::Type::SPLIT});
  // Unwrap tabs from tree nodes or remove from other groups, then add to
  // target group.
  std::vector<std::variant<std::unique_ptr<tabs::TabInterface>,
                           std::unique_ptr<tabs::TabCollection>>>
      owned_tab_or_collection;
  for (auto& tab_or_collection : compacted_moving_tabs) {
    std::visit(
        absl::Overload(
            [&, this](tabs::TabInterface* tab) {
              if (group_collection == tab->GetParentCollection()) {
                // This can happen when creating split with a tab belong
                // to a group. In order to bypass OnGroupEmpty(),
                // directly remove the tab from the group.
                owned_tab_or_collection.push_back(
                    collection_->GetParentCollection(tab, GetPassKey())
                        ->MaybeRemoveTab(tab));
                return;
              }
              owned_tab_or_collection.push_back(DetachTabFromParent(tab));
            },
            [&, this](tabs::TabCollection* collection) {
              CHECK_EQ(collection->type(), tabs::TabCollection::Type::SPLIT);
              owned_tab_or_collection.push_back(DetachSplitFromParent(
                  static_cast<tabs::SplitTabCollection*>(collection)));
            }),
        tab_or_collection);
  }

  // Attach to the target group at 0 index temporarily.
  for (auto& tab : base::Reversed(owned_tab_or_collection)) {
    std::visit(absl::Overload{
                   [&](std::unique_ptr<tabs::TabInterface>&& tab) {
                     auto* tab_ptr = tab.get();
                     group_collection->AddTab(std::move(tab), 0);
                     CHECK(tab_ptr->GetGroup().has_value());
                     CHECK_EQ(tab_ptr->GetGroup().value(),
                              group_collection->GetTabGroupId());
                   },
                   [&](std::unique_ptr<tabs::TabCollection>&& collection) {
                     auto* collection_ptr = collection.get();
                     group_collection->AddCollection(std::move(collection), 0);
                     CHECK_EQ(collection_ptr->type(),
                              tabs::TabCollection::Type::SPLIT);
                     auto tabs = collection_ptr->GetTabsRecursive();
                     CHECK_EQ(tabs.size(), 2u);
                     CHECK_EQ(tabs[0]->GetGroup().value(),
                              group_collection->GetTabGroupId());
                     CHECK_EQ(tabs[1]->GetGroup().value(),
                              group_collection->GetTabGroupId());
                   },
               },
               std::move(tab));
  }

  const int first_in_group = *collection_->GetIndexOfTabRecursive(std::visit(
      absl::Overload{
          [&](const std::unique_ptr<tabs::TabInterface>& tab) {
            return tab.get();
          },
          [&](const std::unique_ptr<tabs::TabCollection>& collection) {
            return collection->GetTabAtIndexRecursive(0);
          },
      },
      std::move(group_collection->GetChildren()[0])));
  std::vector<int> new_indices;
  for (size_t i = 0; i < moving_tabs.size(); ++i) {
    new_indices.push_back(first_in_group + i);
  }
  collection_->MoveTabsRecursiveForDelegate(
      new_indices, destination_index, new_group_id, false,
      retain_collection_types, GetPassKey());
}

std::unique_ptr<tabs::TabInterface>
BraveTreeTabStripCollectionDelegate::DetachTabFromParent(
    tabs::TabInterface* tab) const {
  tabs::TabCollection* parent =
      collection_->GetParentCollection(tab, GetPassKey());
  CHECK(parent);
  if (parent->type() == tabs::TabCollection::Type::GROUP) {
    return DetachTabOutOfGroup(tab);
  }

  // We should unwrap the tab from the tree node and add it to the
  // group.
  CHECK_EQ(parent->type(), tabs::TabCollection::Type::TREE_NODE);
  auto* tree_node = static_cast<tabs::TreeTabNodeTabCollection*>(parent);
  const tree_tab::TreeTabNodeId node_id = tree_node->node().id();
  CHECK(tree_tab_model_);
  tree_tab_model_->RemoveTreeTabNode(node_id);

  MoveChildrenOfTreeTabNodeToParent(tree_node);
  std::unique_ptr<tabs::TabInterface> owned_tab =
      tree_node->MaybeRemoveTab(tab);
  CHECK(owned_tab);
  CHECK_EQ(tree_node->ChildCount(), 0u) << "Tree node should have no children";

  tabs::TabCollection* owner = tree_node->GetParentCollection();
  std::ignore = owner->MaybeRemoveCollection(tree_node);

  return owned_tab;
}

std::unique_ptr<tabs::TabCollection>
BraveTreeTabStripCollectionDelegate::DetachSplitFromParent(
    tabs::SplitTabCollection* collection) {
  CHECK(collection);
  if (collection->GetParentCollection()->type() ==
      tabs::TabCollection::Type::TREE_NODE) {
    return UnwrapTreeNodeForCollection(collection);
  }

  if (collection->GetParentCollection()->type() ==
      tabs::TabCollection::Type::GROUP) {
    auto* group = static_cast<tabs::TabGroupTabCollection*>(
        collection->GetParentCollection());
    auto owned_split_collection = group->MaybeRemoveCollection(collection);
    if (group->TabCountRecursive() == 0) {
      OnGroupEmpty(group);
    }
    return owned_split_collection;
  }

  if (collection->GetParentCollection()->type() ==
      tabs::TabCollection::Type::PINNED) {
    return collection->GetParentCollection()->MaybeRemoveCollection(collection);
  }

  NOTREACHED();
}

std::unique_ptr<tabs::TabInterface>
BraveTreeTabStripCollectionDelegate::DetachTabOutOfGroup(
    tabs::TabInterface* tab) const {
  tabs::TabCollection* parent =
      collection_->GetParentCollection(tab, GetPassKey());
  CHECK_EQ(parent->type(), tabs::TabCollection::Type::GROUP);
  auto* group_collection = static_cast<tabs::TabGroupTabCollection*>(parent);

  std::unique_ptr<tabs::TabInterface> owned_tab =
      group_collection->MaybeRemoveTab(tab);
  CHECK(owned_tab);

  if (group_collection->TabCountRecursive() == 0) {
    OnGroupEmpty(group_collection);
  }

  return owned_tab;
}

void BraveTreeTabStripCollectionDelegate::OnGroupEmpty(
    tabs::TabGroupTabCollection* group_collection) const {
  tabs::TabCollection* group_parent = group_collection->GetParentCollection();
  const bool group_was_wrapped_in_tree_node =
      group_parent &&
      group_parent->type() == tabs::TabCollection::Type::TREE_NODE;
  if (!group_was_wrapped_in_tree_node) {
    return;
  }

  auto* wrapper = static_cast<tabs::TreeTabNodeTabCollection*>(group_parent);
  MoveChildrenOfTreeTabNodeToParent(wrapper);

  // Notify the model before removing the wrapper so observers (e.g.
  // BraveBrowserTabStripController::OnTreeTabChanged) can safely call
  // node->GetTabs() while the wrapper and group are still in the
  // hierarchy. The wrapper destructor will call RemoveTreeTabNode again,
  // which is a no-op because the node is already removed from the model.
  CHECK(tree_tab_model_);
  tree_tab_model_->RemoveTreeTabNode(wrapper->node().id());

  auto removed_group = collection_->RemoveTabCollection(group_collection);
  CHECK(removed_group);

  // This looks weird, but we need to call this api so that extend the
  // lifecycle of the group collection, until TabStripModel would close
  // this group at the right time.
  collection_->CreateTabGroup(base::WrapUnique<tabs::TabGroupTabCollection>(
      static_cast<tabs::TabGroupTabCollection*>(removed_group.release())));

  std::ignore = wrapper->GetParentCollection()->MaybeRemoveCollection(wrapper);
}

void BraveTreeTabStripCollectionDelegate::MoveTabsOutOfGroup(
    const std::vector<tabs::TabInterface*>& moving_tabs,
    const base::flat_set<tab_groups::TabGroupId>& moving_groups,
    size_t destination_index,
    bool new_pinned_state,
    const tabs::TabCollection::TypeEnumSet& retain_collection_types) {
  CHECK(!moving_tabs.empty());
  auto compacted_moving_tabs =
      CompactMovingTabs(moving_tabs, {tabs::TabCollection::Type::SPLIT});
  if (new_pinned_state) {
    // In this case, we just move the tabs to the pinned collection.
    for (auto& tab_or_collection : base::Reversed(compacted_moving_tabs)) {
      std::visit(absl::Overload{
                     [&](tabs::TabInterface* tab) {
                       auto detached_tab = DetachTabOutOfGroup(tab);
                       CHECK(detached_tab);
                       collection_->AddTabRecursive(
                           std::move(detached_tab), destination_index,
                           /*new_group_id=*/std::nullopt,
                           /*new_pinned_state=*/true, GetPassKey());
                     },
                     [&](tabs::TabCollection* collection) {
                       CHECK_EQ(collection->type(),
                                tabs::TabCollection::Type::SPLIT);
                       auto* tab_0 = collection->GetTabAtIndexRecursive(0);
                       auto* tab_1 = collection->GetTabAtIndexRecursive(1);
                       PinSplit(tab_0, {tab_0, tab_1}, destination_index);
                     },
                 },
                 tab_or_collection);
    }
    return;
  }

  // We'll detach the the the tabs/splits from the group and add to the unpinned
  // collection temporarily.
  auto* unpinned_collection = collection_->unpinned_collection();
  for (auto& tab_or_collection : compacted_moving_tabs) {
    std::visit(
        absl::Overload(
            [&](tabs::TabInterface* tab) {
              auto group_id = tab->GetGroup();
              if (group_id && moving_groups.contains(*group_id)) {
                // In this case we should not detach the tab from the group
                return;
              }

              std::unique_ptr<tabs::TreeTabNodeTabCollection> tree_node;
              bool is_new_tree_node = false;
              if (group_id) {
                is_new_tree_node = true;
                std::unique_ptr<tabs::TabInterface> detached_tab;
                detached_tab = DetachTabOutOfGroup(tab);
                tree_node = std::make_unique<tabs::TreeTabNodeTabCollection>(
                    tree_tab::TreeTabNodeId::GenerateNew(),
                    std::move(detached_tab),
                    base::BindRepeating(&TreeTabModel::RemoveTreeTabNode,
                                        tree_tab_model_),
                    base::BindRepeating(&TreeTabModel::OnTreeTabNodeMoved,
                                        tree_tab_model_));
              } else {
                // Can happen during multi-select - ungrouped tab was selected.
                auto* parent =
                    collection_->GetParentCollection(tab, GetPassKey());
                CHECK_EQ(parent->type(), tabs::TabCollection::Type::TREE_NODE);
                tree_node = base::WrapUnique<tabs::TreeTabNodeTabCollection>(
                    static_cast<tabs::TreeTabNodeTabCollection*>(
                        parent->GetParentCollection()
                            ->MaybeRemoveCollection(parent)
                            .release()));
              }
              CHECK(tree_node);

              auto* tree_node_ptr = tree_node.get();
              const auto new_index = unpinned_collection->ChildCount();
              collection_->AddTabCollectionAtPosition(
                  std::move(tree_node),
                  tabs::TabCollection::Position{
                      unpinned_collection->GetHandle(), new_index},
                  GetPassKey());

              if (is_new_tree_node) {
                tree_tab_model_->AddTreeTabNode(tree_node_ptr->node());
              }
            },
            [&](tabs::TabCollection* collection) {
              CHECK_EQ(collection->type(), tabs::TabCollection::Type::SPLIT);
              auto* parent = collection->GetParentCollection();
              if (parent->type() != tabs::TabCollection::Type::GROUP) {
                CHECK_EQ(parent->type(), tabs::TabCollection::Type::TREE_NODE);
                // In this case we should not detach the split from the group.
                // Ungrouped split was selected via multi-select. We just move
                // it.
                collection_->AddTabCollectionAtPosition(
                    parent->GetParentCollection()->MaybeRemoveCollection(
                        parent),
                    tabs::TabCollection::Position{
                        unpinned_collection->GetHandle(),
                        unpinned_collection->ChildCount()},
                    GetPassKey());
                return;
              }

              auto group_id = static_cast<tabs::TabGroupTabCollection*>(
                                  collection->GetParentCollection())
                                  ->GetTabGroupId();
              if (moving_groups.contains(group_id)) {
                // In this case we should not detach the split from the group.
                // The split is already in the moving groups.
                return;
              }

              auto owned_split_collection =
                  base::WrapUnique<tabs::SplitTabCollection>(
                      static_cast<tabs::SplitTabCollection*>(
                          DetachSplitFromParent(
                              static_cast<tabs::SplitTabCollection*>(
                                  collection))
                              .release()));
              WrapCollectionInTreeNodeAndInsert(
                  std::move(owned_split_collection),
                  collection_->TabCountRecursive(), false, std::nullopt);
            }),
        std::move(tab_or_collection));
  }

  // As the loop above may have affected the indices of the tabs, we need to
  // recalculate the indices - we can't calculate indices in the middle of the
  // loop above.
  std::vector<int> new_indices;
  std::ranges::transform(
      moving_tabs, std::back_inserter(new_indices), [&, this](auto* tab) {
        return collection_->GetIndexOfTabRecursive(tab).value();
      });
  std::ranges::sort(new_indices);

  // This will reach the the last part of MoveTabsRecursive() which moves
  // tree nodes and groups to the destination index.
  MoveTabsRecursive(new_indices, destination_index, {}, false,
                    retain_collection_types);
}

void BraveTreeTabStripCollectionDelegate::PinTabs(
    const std::vector<tabs::TabInterface*>& moving_tabs,
    size_t destination_index) {
  // Note that we're using std::set instead of base::flat_set because
  // TabCollection::FindMovePositionRecursive() expects a set of tabs and
  // collections to be passed in.
  base::flat_map<split_tabs::SplitTabId, std::set<tabs::TabInterface*>>
      split_tabs;
  for (auto* moving_tab : base::Reversed(moving_tabs)) {
    auto* parent_collection =
        collection_->GetParentCollection(moving_tab, GetPassKey());
    if (parent_collection->type() == tabs::TabCollection::Type::PINNED) {
      auto detached_tab = parent_collection->MaybeRemoveTab(moving_tab);
      CHECK(detached_tab);
      collection_->AddTabRecursive(std::move(detached_tab), destination_index,
                                   /*new_group_id=*/std::nullopt,
                                   /*new_pinned_state=*/true, GetPassKey());
      continue;
    }

    if (parent_collection->type() == tabs::TabCollection::Type::TREE_NODE) {
      // Unwraps the tree node and add the tab to the pinned collection.
      auto* tree_node_collection =
          static_cast<tabs::TreeTabNodeTabCollection*>(parent_collection);
      tree_tab_model_->RemoveTreeTabNode(tree_node_collection->node().id());
      MoveChildrenOfTreeTabNodeToParent(tree_node_collection);

      auto detached_tab = parent_collection->MaybeRemoveTab(moving_tab);
      CHECK(detached_tab);
      collection_->AddTabRecursive(std::move(detached_tab), destination_index,
                                   /*new_group_id=*/std::nullopt,
                                   /*new_pinned_state=*/true, GetPassKey());

      // Destroys the wrapper.
      std::ignore =
          tree_node_collection->GetParentCollection()->MaybeRemoveCollection(
              tree_node_collection);
      continue;
    }

    if (parent_collection->type() == tabs::TabCollection::Type::SPLIT) {
      auto split_id = moving_tab->GetSplit();
      CHECK(split_id.has_value());

      split_tabs[split_id.value()].insert(moving_tab);
      if (split_tabs[split_id.value()].size() < 2) {
        continue;
      }

      PinSplit(moving_tab, split_tabs[split_id.value()], destination_index);
      continue;
    }

    if (parent_collection->type() == tabs::TabCollection::Type::GROUP) {
      NOTREACHED()
          << "This case should be handled in the MoveTabsOutOfGroup() above";
    }

    NOTREACHED();
  }
}

void BraveTreeTabStripCollectionDelegate::PinSplit(
    tabs::TabInterface* moving_tab,
    const std::set<tabs::TabInterface*>& split_tab_set,
    size_t destination_index) {
  // Split tabs are pinned/unpinned together, so we need to move them
  // together. First unwraps the split collection.
  auto* split_collection =
      collection_->GetParentCollection(moving_tab, GetPassKey());
  CHECK_EQ(split_collection->type(), tabs::TabCollection::Type::SPLIT);

  std::unique_ptr<tabs::TabCollection> owned_split_collection =
      DetachSplitFromParent(
          static_cast<tabs::SplitTabCollection*>(split_collection));
  CHECK(owned_split_collection);

  size_t curr_insertion_index = 0u;
  auto insert_position = collection_->FindMovePositionRecursive(
      destination_index, collection_->pinned_collection(), curr_insertion_index,
      split_tab_set, {owned_split_collection.get()});
  CHECK(insert_position.has_value());
  CHECK_EQ(insert_position->parent_handle.Get(),
           collection_->pinned_collection());
  CHECK_LE(insert_position->index,
           collection_->pinned_collection()->ChildCount());

  collection_->AddTabCollectionAtPosition(
      std::move(owned_split_collection), insert_position.value(), GetPassKey());
}

void BraveTreeTabStripCollectionDelegate::UnpinSplit(
    tabs::TabInterface* moving_tab) const {
  // Split tabs are pinned/unpinned together, so we need to move them
  // together. Wraps the split collection with a tree node and add to the
  // unpinned collection.
  auto* split_collection =
      collection_->GetParentCollection(moving_tab, GetPassKey());
  CHECK_EQ(split_collection->type(), tabs::TabCollection::Type::SPLIT);
  CHECK(split_collection->GetParentCollection());

  auto owned_split_collection = base::WrapUnique<tabs::SplitTabCollection>(
      static_cast<tabs::SplitTabCollection*>(
          split_collection->GetParentCollection()
              ->MaybeRemoveCollection(split_collection)
              .release()));
  CHECK(owned_split_collection);

  auto tree_node_collection = std::make_unique<tabs::TreeTabNodeTabCollection>(
      tree_tab::TreeTabNodeId::GenerateNew(), std::move(owned_split_collection),
      base::BindRepeating(&TreeTabModel::RemoveTreeTabNode, tree_tab_model_),
      base::BindRepeating(&TreeTabModel::OnTreeTabNodeMoved, tree_tab_model_));
  auto* tree_node_ptr = tree_node_collection.get();

  // Temporarily attach to unpinned collection's first position.
  collection_->unpinned_collection()->AddCollection(
      std::move(tree_node_collection), 0);

  CHECK(tree_tab_model_);
  tree_tab_model_->AddTreeTabNode(tree_node_ptr->node());
}

void BraveTreeTabStripCollectionDelegate::UnpinTabs(
    const std::vector<tabs::TabInterface*>& moving_tabs,
    size_t destination_index,
    std::optional<tab_groups::TabGroupId> new_group_id,
    const tabs::TabCollection::TypeEnumSet retain_collection_types) {
  std::vector<int> new_tab_indices;
  base::flat_map<split_tabs::SplitTabId, std::vector<tabs::TabInterface*>>
      split_tabs;

  for (auto* moving_tab : base::Reversed(moving_tabs)) {
    if (!IsTabInPinnedCollection(moving_tab)) {
      // Already in unpinned collection. This will be moved together with
      // MoveTabsRecursive() call in the end of this function.
      continue;
    }

    if (auto split_id = moving_tab->GetSplit()) {
      split_tabs[split_id.value()].push_back(moving_tab);
      if (split_tabs[split_id.value()].size() != 2) {
        continue;
      }
      // Tabs in split should be moved together.
      UnpinSplit(moving_tab);
      continue;
    }

    // Detach from the original parent collection - pinned collection.
    auto detached_tab =
        collection_->GetParentCollection(moving_tab, GetPassKey())
            ->MaybeRemoveTab(moving_tab);
    CHECK(detached_tab);

    // Wraps with tree node.
    auto tree_tab_node = std::make_unique<tabs::TreeTabNodeTabCollection>(
        tree_tab::TreeTabNodeId::GenerateNew(), std::move(detached_tab),
        base::BindRepeating(&TreeTabModel::RemoveTreeTabNode, tree_tab_model_),
        base::BindRepeating(&TreeTabModel::OnTreeTabNodeMoved,
                            tree_tab_model_));
    auto* tree_tab_node_ptr = tree_tab_node.get();
    // Temporarily attach to unpinned collection's first position.
    collection_->unpinned_collection()->AddCollection(std::move(tree_tab_node),
                                                      0);

    CHECK(tree_tab_model_);
    tree_tab_model_->AddTreeTabNode(tree_tab_node_ptr->node());
  }

  for (auto* moving_tab : moving_tabs) {
    new_tab_indices.push_back(
        collection_->GetIndexOfTabRecursive(moving_tab).value());
  }

  // The indices should be sorted in ascending order.
  std::ranges::sort(new_tab_indices);
  MoveTabsRecursive(new_tab_indices, destination_index, new_group_id,
                    /*new_pinned_state=*/false, retain_collection_types);
}

bool BraveTreeTabStripCollectionDelegate::IsTabInPinnedCollection(
    tabs::TabInterface* tab) const {
  CHECK(tab);
  auto* parent = tab->GetParentCollection();
  if (!parent) {
    return false;
  }

  if (parent->type() == tabs::TabCollection::Type::SPLIT) {
    parent = parent->GetParentCollection();
  }

  return parent && parent->type() == tabs::TabCollection::Type::PINNED;
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

  if (IsTabInPinnedCollection(tabs[0])) {
    CHECK(IsTabInPinnedCollection(tabs[1]));
    // Let the upstream handle this case as there's no tree specific handling
    // for splits in pinned collections.
    return false;
  }

  if (tabs[0]->GetGroup()) {
    CHECK_EQ(tabs[0]->GetGroup().value(), tabs[1]->GetGroup().value());
    // Let the upstream handle this case. In this case, split belongs to a
    // group and we don't have tree node between the split and the group.
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

bool BraveTreeTabStripCollectionDelegate::Unsplit(
    split_tabs::SplitTabId split_id) {
  tabs::SplitTabCollection* split =
      collection_->GetSplitTabCollection(split_id);
  if (!split || collection_->pinned_collection()->GetIndexOfCollection(split)) {
    return false;
  }

  tabs::TabCollection* wrapper = split->GetParentCollection();
  if (wrapper->type() == tabs::TabCollection::Type::GROUP) {
    // Let the upstream handle this case. This is a case where the split
    // belongs to a group and we don't have tree node between the split and
    // the group.
    return false;
  }

  // The split's parent is the wrapper tree node; we will destroy it too.
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
  return true;
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

const tree_tab::TreeTabNodeId*
BraveTreeTabStripCollectionDelegate::GetTreeTabNodeIdForGroup(
    tab_groups::TabGroupId group_id) const {
  tabs::TabGroupTabCollection* group_collection =
      collection_->GetTabGroupCollection(group_id);
  if (!group_collection) {
    return nullptr;
  }
  tabs::TabCollection* parent = group_collection->GetParentCollection();
  if (!parent || parent->type() != tabs::TabCollection::Type::TREE_NODE) {
    return nullptr;
  }
  return &static_cast<tabs::TreeTabNodeTabCollection*>(parent)->node().id();
}
