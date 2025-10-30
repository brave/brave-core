// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TABS_PUBLIC_BRAVE_TAB_STRIP_COLLECTION_H_
#define BRAVE_COMPONENTS_TABS_PUBLIC_BRAVE_TAB_STRIP_COLLECTION_H_

#include "components/tabs/public/tab_strip_collection.h"

namespace tabs {

class BraveTabStripCollectionDelegate;

// BraveTabStripCollection is used to override methods for tree tabs.
// * In AddTabRecursive(), we need to to create TreeTabNode with a given new tab
//   and insert it into target collection. The target collection can be another
//   TreeTabNode in case new tab has opener relationship with the previous tab.
//   Otherwise, the target collection can be anything but pinned collection.
class BraveTabStripCollection : public TabStripCollection {
 public:
  BraveTabStripCollection();
  ~BraveTabStripCollection() override;

  void SetDelegate(std::unique_ptr<BraveTabStripCollectionDelegate> delegate);

  // Exposes APIs for delegate to override behaviors.
  tabs::TabCollection* GetParentCollection(
      TabInterface* tab,
      base::PassKey<BraveTabStripCollectionDelegate> pass_key) const;
  void AddTabRecursive(
      std::unique_ptr<TabInterface> tab,
      size_t index,
      std::optional<tab_groups::TabGroupId> new_group_id,
      bool new_pinned_state,
      base::PassKey<BraveTabStripCollectionDelegate> pass_key);

  // TabStripCollection:
  void AddTabRecursive(std::unique_ptr<TabInterface> tab,
                       size_t index,
                       std::optional<tab_groups::TabGroupId> new_group_id,
                       bool new_pinned_state,
                       TabInterface* opener) override;
  void MoveTabRecursive(size_t initial_index,
                        size_t final_index,
                        std::optional<tab_groups::TabGroupId> new_group_id,
                        bool new_pinned_state) override;
  void MoveTabsRecursive(
      const std::vector<int>& tab_indices,
      size_t destination_index,
      std::optional<tab_groups::TabGroupId> new_group_id,
      bool new_pinned_state,
      const std::set<TabCollection::Type>& retain_collection_types) override;
  std::unique_ptr<TabInterface> RemoveTabAtIndexRecursive(
      size_t index) override;
  std::unique_ptr<TabInterface> RemoveTabRecursive(
      TabInterface* tab,
      bool close_empty_group_collection) override;

 private:
  std::unique_ptr<BraveTabStripCollectionDelegate> delegate_;
};

}  // namespace tabs

#endif  // BRAVE_COMPONENTS_TABS_PUBLIC_BRAVE_TAB_STRIP_COLLECTION_H_
