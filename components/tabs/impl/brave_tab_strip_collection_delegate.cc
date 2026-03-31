// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/tabs/public/brave_tab_strip_collection_delegate.h"

#include "components/split_tabs/split_tab_visual_data.h"

namespace tabs {

BraveTabStripCollectionDelegate::BraveTabStripCollectionDelegate(
    BraveTabStripCollection& collection)
    : collection_(collection) {}

BraveTabStripCollectionDelegate::~BraveTabStripCollectionDelegate() = default;

base::PassKey<BraveTabStripCollectionDelegate>
BraveTabStripCollectionDelegate::GetPassKey() const {
  return {};
}

bool BraveTabStripCollectionDelegate::CreateSplit(
    split_tabs::SplitTabId split_id,
    const std::vector<TabInterface*>& tabs,
    split_tabs::SplitTabVisualData visual_data) const {
  return false;
}

bool BraveTabStripCollectionDelegate::Unsplit(split_tabs::SplitTabId split_id) {
  return false;
}

tabs::TabCollection* BraveTabStripCollectionDelegate::GetCollectionForMapping(
    tabs::TabCollection* root_collection) {
  return root_collection;
}

const tree_tab::TreeTabNodeId*
BraveTabStripCollectionDelegate::GetTreeTabNodeIdForGroup(
    tab_groups::TabGroupId group_id) const {
  return nullptr;
}

}  // namespace tabs
