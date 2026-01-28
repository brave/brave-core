// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/tabs/public/brave_tab_strip_collection.h"

#include "brave/components/tabs/public/brave_tab_strip_collection_delegate.h"
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

void BraveTabStripCollection::AddTabRecursive(
    std::unique_ptr<TabInterface> tab,
    size_t index,
    std::optional<tab_groups::TabGroupId> new_group_id,
    bool new_pinned_state) {
  if (delegate_ && delegate_->ShouldHandleTabManipulation()) {
    delegate_->AddTabRecursive(std::move(tab), index, new_group_id,
                               new_pinned_state, /*opener*/ nullptr);
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

}  // namespace tabs
