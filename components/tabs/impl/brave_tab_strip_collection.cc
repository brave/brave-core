// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/tabs/public/brave_tab_strip_collection.h"

#include "brave/components/tabs/public/brave_tab_strip_collection_delegate.h"
#include "components/split_tabs/split_tab_visual_data.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_interface.h"

namespace tabs {

// We disable |send_notifications_immediately| in order to be aligned with
// the original TabStripCollection creation in TabStripModel.
BraveTabStripCollection::BraveTabStripCollection()
    : TabStripCollection(/*send_notifications_immediately=*/false) {}

BraveTabStripCollection::~BraveTabStripCollection() = default;

void BraveTabStripCollection::SetDelegate(
    std::unique_ptr<BraveTabStripCollectionDelegate> delegate) {
  CHECK_NE(delegate_, delegate);
  delegate_ = std::move(delegate);
}

tabs::TabCollection* BraveTabStripCollection::GetParentCollection(
    TabInterface* tab,
    base::PassKey<BraveTabStripCollectionDelegate> pass_key) const {
  return tab->GetParentCollection(GetPassKey());
}

const ChildrenVector& BraveTabStripCollection::GetChildrenForDelegate(
    const TabCollection& collection,
    base::PassKey<BraveTabStripCollectionDelegate> pass_key) const {
  return GetChildrenStatic(collection);
}

void BraveTabStripCollection::AddTabRecursive(
    std::unique_ptr<TabInterface> tab,
    size_t index,
    std::optional<tab_groups::TabGroupId> new_group_id,
    bool new_pinned_state,
    base::PassKey<BraveTabStripCollectionDelegate> pass_key) {
  TabStripCollection::AddTabRecursive(std::move(tab), index, new_group_id,
                                      new_pinned_state);
}

std::unique_ptr<TabInterface>
BraveTabStripCollection::RemoveTabAtIndexRecursive(
    size_t index,
    base::PassKey<BraveTabStripCollectionDelegate> pass_key) {
  return TabStripCollection::RemoveTabAtIndexRecursive(index);
}

void BraveTabStripCollection::AddTabRecursive(
    std::unique_ptr<TabInterface> tab,
    size_t index,
    std::optional<tab_groups::TabGroupId> new_group_id,
    bool new_pinned_state,
    TabInterface* opener) {
  if (delegate_ && delegate_->ShouldHandleTabManipulation()) {
    delegate_->AddTabRecursive(std::move(tab), index, new_group_id,
                               new_pinned_state, opener);
    return;
  }

  TabStripCollection::AddTabRecursive(std::move(tab), index, new_group_id,
                                      new_pinned_state);
}

void BraveTabStripCollection::MoveTabsRecursive(
    const std::vector<int>& tab_indices,
    size_t destination_index,
    std::optional<tab_groups::TabGroupId> new_group_id,
    bool new_pinned_state,
    const TabCollection::TypeEnumSet retain_collection_types) {
  if (delegate_ && delegate_->ShouldHandleTabManipulation()) {
    delegate_->MoveTabsRecursive(tab_indices, destination_index, new_group_id,
                                 new_pinned_state, retain_collection_types);
    return;
  }

  TabStripCollection::MoveTabsRecursive(tab_indices, destination_index,
                                        new_group_id, new_pinned_state,
                                        retain_collection_types);
}

std::unique_ptr<TabInterface>
BraveTabStripCollection::RemoveTabAtIndexRecursive(size_t index) {
  if (delegate_ && delegate_->ShouldHandleTabManipulation()) {
    return delegate_->RemoveTabAtIndexRecursive(index);
  }

  return TabStripCollection::RemoveTabAtIndexRecursive(index);
}

void BraveTabStripCollection::AddTabCollectionAtPosition(
    std::unique_ptr<TabCollection> collection,
    const TabCollection::Position& position,
    base::PassKey<BraveTabStripCollectionDelegate> pass_key) {
  TabStripCollection::AddTabCollectionAtPosition(std::move(collection),
                                                 position);
}

std::unique_ptr<TabGroupTabCollection>
BraveTabStripCollection::PopDetachedGroupCollectionForDelegate(
    tab_groups::TabGroupId group_id,
    base::PassKey<BraveTabStripCollectionDelegate> pass_key) {
  return PopDetachedGroupCollection(group_id);
}

void BraveTabStripCollection::CreateSplit(
    split_tabs::SplitTabId split_id,
    const std::vector<TabInterface*>& tabs,
    split_tabs::SplitTabVisualData visual_data) {
  if (delegate_ && delegate_->ShouldHandleTabManipulation() &&
      delegate_->CreateSplit(split_id, tabs, visual_data)) {
    return;
  }
  TabStripCollection::CreateSplit(split_id, tabs, visual_data);
}

void BraveTabStripCollection::Unsplit(split_tabs::SplitTabId split_id) {
  if (delegate_ && delegate_->ShouldHandleTabManipulation() &&
      delegate_->Unsplit(split_id)) {
    return;
  }
  TabStripCollection::Unsplit(split_id);
}

void BraveTabStripCollection::AddCollectionMapping(
    TabCollection* root_collection) {
  if (delegate_ && delegate_->ShouldHandleTabManipulation()) {
    TabStripCollection::AddCollectionMapping(
        delegate_->GetCollectionForMapping(root_collection));
    return;
  }

  TabStripCollection::AddCollectionMapping(root_collection);
}

void BraveTabStripCollection::RemoveCollectionMapping(
    TabCollection* root_collection) {
  if (delegate_ && delegate_->ShouldHandleTabManipulation()) {
    TabStripCollection::RemoveCollectionMapping(
        delegate_->GetCollectionForMapping(root_collection));
    return;
  }

  TabStripCollection::RemoveCollectionMapping(root_collection);
}

const tree_tab::TreeTabNodeId*
BraveTabStripCollection::GetTreeTabNodeIdForGroup(
    tab_groups::TabGroupId group_id) const {
  if (delegate_ && delegate_->ShouldHandleTabManipulation()) {
    return delegate_->GetTreeTabNodeIdForGroup(group_id);
  }

  return nullptr;
}

}  // namespace tabs
