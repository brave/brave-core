// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <components/tabs/impl/tab_strip_collection.cc>

namespace tabs {

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
