// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TABS_BRAVE_TREE_TAB_STRIP_COLLECTION_DELEGATE_H_
#define BRAVE_BROWSER_UI_TABS_BRAVE_TREE_TAB_STRIP_COLLECTION_DELEGATE_H_

#include "brave/components/tabs/public/brave_tab_strip_collection_delegate.h"

class TreeTabModel;

// A delegate for BraveTabStripCollection when in tree tab mode.
// This class do pre/post processing for tab manipulation methods so that we can
// keep tabs in a valid tree tab structure.
class BraveTreeTabStripCollectionDelegate
    : public tabs::BraveTabStripCollectionDelegate {
 public:
  BraveTreeTabStripCollectionDelegate(
      tabs::BraveTabStripCollection& collection,
      base::WeakPtr<TreeTabModel> tree_tab_model);
  ~BraveTreeTabStripCollectionDelegate() override;

  // tabs::BraveTabStripCollectionDelegate:
  bool ShouldHandleTabManipulation() const override;
  void AddTabRecursive(std::unique_ptr<tabs::TabInterface> tab,
                       size_t index,
                       std::optional<tab_groups::TabGroupId> new_group_id,
                       bool new_pinned_state,
                       tabs::TabInterface* opener) const override;
  std::unique_ptr<tabs::TabInterface> RemoveTabAtIndexRecursive(
      size_t index) const override;
  void MoveTabsRecursive(const std::vector<int>& tab_indices,
                         size_t destination_index,
                         std::optional<tab_groups::TabGroupId> new_group_id,
                         bool new_pinned_state,
                         const std::set<tabs::TabCollection::Type>&
                             retain_collection_types) const override;

 private:
  bool in_destruction_ = false;

  base::WeakPtr<TreeTabModel> tree_tab_model_;
};

#endif  // BRAVE_BROWSER_UI_TABS_BRAVE_TREE_TAB_STRIP_COLLECTION_DELEGATE_H_
