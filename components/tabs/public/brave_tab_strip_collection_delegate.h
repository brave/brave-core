// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_TAB_STRIP_COLLECTION_DELEGATE_H_
#define BRAVE_TAB_STRIP_COLLECTION_DELEGATE_H_

#include "base/types/pass_key.h"
#include "brave/components/tabs/public/brave_tab_strip_collection.h"

namespace tabs {

class BraveTabStripCollection;

// Delegate TabStripCollection's behavior via this interface. This gives room
// for customizing actions in a certain situation, such as tree tab mode is on.
class BraveTabStripCollectionDelegate {
 public:
  explicit BraveTabStripCollectionDelegate(BraveTabStripCollection& collection);
  BraveTabStripCollectionDelegate(const BraveTabStripCollectionDelegate&) =
      delete;
  BraveTabStripCollectionDelegate& operator=(
      const BraveTabStripCollectionDelegate&) = delete;
  virtual ~BraveTabStripCollectionDelegate();

  base::PassKey<BraveTabStripCollectionDelegate> GetPassKey() const;

  // Returns true if this delegate wants to handle tab manipulation actions
  // such as adding/moving/removing tabs.
  virtual bool ShouldHandleTabManipulation() const = 0;

  virtual void AddTabRecursive(
      std::unique_ptr<TabInterface> tab,
      size_t index,
      std::optional<tab_groups::TabGroupId> new_group_id,
      bool new_pinned_state,
      TabInterface* opener) const = 0;
  virtual std::unique_ptr<TabInterface> RemoveTabAtIndexRecursive(
      size_t index) const = 0;
  virtual void MoveTabsRecursive(
      const std::vector<int>& tab_indices,
      size_t destination_index,
      std::optional<tab_groups::TabGroupId> new_group_id,
      bool new_pinned_state,
      const std::set<TabCollection::Type>& retain_collection_types) const = 0;

 protected:
  // owner of this delegate.
  raw_ref<BraveTabStripCollection> collection_;
};

}  // namespace tabs

#endif  // BRAVE_TAB_STRIP_COLLECTION_DELEGATE_H_
