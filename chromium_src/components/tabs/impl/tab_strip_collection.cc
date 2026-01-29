// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/tabs/public/tab_strip_collection.h"

#define RemoveTabAtIndexRecursive RemoveTabAtIndexRecursive_Chromium

#include <components/tabs/impl/tab_strip_collection.cc>

#undef RemoveTabAtIndexRecursive

namespace tabs {

std::unique_ptr<TabInterface> TabStripCollection::RemoveTabAtIndexRecursive(
    size_t index) {
#if !BUILDFLAG(IS_ANDROID)
  TabInterface* tab_to_be_removed = GetTabAtIndexRecursive(index);
  TabCollection* parent_collection =
      tab_to_be_removed->GetParentCollection(GetPassKey());
  CHECK(parent_collection);
  if (parent_collection->type() == TabCollection::Type::TREE_NODE) {
    // When it's tree node, bypass upstream's implementation so that we can
    // avoid crash from RemoveTabCollectionImpl(parent_collection). Calling this
    // will destroy the storage and the tab in the storage, but we should return
    // the tab for further processing.
    // TODO(https://github.com/brave/brave-browser/issues/49789) This should be
    // revisited once tab removal cases are fully handled for tree tabs.
    auto tab = RemoveTabImpl(tab_to_be_removed);

    if (auto* grand_parent = parent_collection->GetParentCollection();
        grand_parent && grand_parent->type() == TabCollection::Type::GROUP &&
        grand_parent->TabCountRecursive() == 0) {
      // If the grand parent is a group, we need to close the tab group when the
      // removed tab was the only descendant tab of the group.
      RemoveTabCollectionImpl(grand_parent);
    }

    return tab;
  }
#endif  // !BUILDFLAG(IS_ANDROID)

  return RemoveTabAtIndexRecursive_Chromium(index);
}

void TabStripCollection::AddTabRecursive(
    std::unique_ptr<TabInterface> tab,
    size_t index,
    std::optional<tab_groups::TabGroupId> new_group_id,
    bool new_pinned_state,
    TabInterface* opener) {
  // Default implementation just calls the base class method without opener.
  // This method will be overriden in BraveTabStripCollection and use opener
  // for tree tab mod
  AddTabRecursive(std::move(tab), index, new_group_id, new_pinned_state);
}

}  // namespace tabs
