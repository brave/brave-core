// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

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

 private:
  bool in_desturction_ = false;

  base::WeakPtr<TreeTabModel> tree_tab_model_;
};
