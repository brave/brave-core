// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/tabs/public/brave_tab_strip_collection.h"

#include "base/logging.h"
#include "base/notimplemented.h"
#include "brave/components/tabs/public/brave_tab_strip_collection_delegate.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_interface.h"

namespace tabs {

BraveTabStripCollection::BraveTabStripCollection() = default;

BraveTabStripCollection::~BraveTabStripCollection() = default;

void BraveTabStripCollection::SetDelegate(
    std::unique_ptr<BraveTabStripCollectionDelegate> delegate) {
  CHECK(!delegate_ || !delegate);
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

void BraveTabStripCollection::MoveTabRecursive(
    size_t initial_index,
    size_t final_index,
    std::optional<tab_groups::TabGroupId> new_group_id,
    bool new_pinned_state) {
  if (delegate_ && delegate_->ShouldHandleTabManipulation()) {
    LOG(ERROR) << initial_index << " -> " << final_index;
    // delegate_->OnBeforeMoveTabRecursive(initial_index, final_index);
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

}  // namespace tabs
