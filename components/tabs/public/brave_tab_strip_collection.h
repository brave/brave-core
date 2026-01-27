// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TABS_PUBLIC_BRAVE_TAB_STRIP_COLLECTION_H_
#define BRAVE_COMPONENTS_TABS_PUBLIC_BRAVE_TAB_STRIP_COLLECTION_H_

#include <cstddef>

#include "components/tabs/public/tab_strip_collection.h"

namespace tabs {

class BraveTabStripCollectionDelegate;

// BraveTabStripCollection is a TabStripCollection that allows a delegate to
// override certain behaviors such as adding, removing, and moving tabs
// recursively. This is useful for implementing custom tab strip behaviors in
// Brave, while still leveraging the existing TabStripCollection logic.
class BraveTabStripCollection : public TabStripCollection {
 public:
  BraveTabStripCollection();
  ~BraveTabStripCollection() override;

  // Sets the delegate used to override tab strip collection behaviors.
  // Ownership of the |delegate| is transferred to the BraveTabStripCollection.
  void SetDelegate(std::unique_ptr<BraveTabStripCollectionDelegate> delegate);

  // Exposes APIs for delegate to override behaviors ---------------------------
  // Theses methods call the default behavior of TabStripCollection for on
  // behalf of the delegate.
  tabs::TabCollection* GetParentCollection(
      TabInterface* tab,
      base::PassKey<BraveTabStripCollectionDelegate> pass_key) const;
  const ChildrenVector& GetChildrenForDelegate(
      const TabCollection& collection,
      base::PassKey<BraveTabStripCollectionDelegate> pass_key) const;
  void AddTabRecursive(std::unique_ptr<TabInterface> tab,
                       size_t index,
                       std::optional<tab_groups::TabGroupId> new_group_id,
                       bool new_pinned_state,
                       base::PassKey<BraveTabStripCollectionDelegate> pass_key);
  std::unique_ptr<TabInterface> RemoveTabAtIndexRecursive(
      size_t index,
      base::PassKey<BraveTabStripCollectionDelegate> pass_key);

  // TabStripCollection:
  void AddTabRecursive(std::unique_ptr<TabInterface> tab,
                       size_t index,
                       std::optional<tab_groups::TabGroupId> new_group_id,
                       bool new_pinned_state,
                       TabInterface* opener) override;
  void MoveTabsRecursive(
      const std::vector<int>& tab_indices,
      size_t destination_index,
      std::optional<tab_groups::TabGroupId> new_group_id,
      bool new_pinned_state,
      const std::set<TabCollection::Type>& retain_collection_types) override;
  std::unique_ptr<TabInterface> RemoveTabAtIndexRecursive(
      size_t index) override;

 private:
  std::unique_ptr<BraveTabStripCollectionDelegate> delegate_;
};

}  // namespace tabs

#endif  // BRAVE_COMPONENTS_TABS_PUBLIC_BRAVE_TAB_STRIP_COLLECTION_H_
