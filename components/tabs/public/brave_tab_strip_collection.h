// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TABS_PUBLIC_BRAVE_TAB_STRIP_COLLECTION_H_
#define BRAVE_COMPONENTS_TABS_PUBLIC_BRAVE_TAB_STRIP_COLLECTION_H_

#include <cstddef>
#include <memory>

#include "components/split_tabs/split_tab_id.h"
#include "components/tab_groups/tab_group_id.h"
#include "components/tabs/public/tab_group_tab_collection.h"
#include "components/tabs/public/tab_strip_collection.h"

namespace split_tabs {
class SplitTabVisualData;
}  // namespace split_tabs

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
  void AddTabCollectionAtPosition(
      std::unique_ptr<TabCollection> collection,
      const TabCollection::Position& position,
      base::PassKey<BraveTabStripCollectionDelegate> pass_key);
  std::unique_ptr<TabGroupTabCollection> PopDetachedGroupCollectionForDelegate(
      tab_groups::TabGroupId group_id,
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
      const TabCollection::TypeEnumSet retain_collection_types) override;
  std::unique_ptr<TabInterface> RemoveTabAtIndexRecursive(
      size_t index) override;
  void CreateSplit(split_tabs::SplitTabId split_id,
                   const std::vector<TabInterface*>& tabs,
                   split_tabs::SplitTabVisualData visual_data) override;
  void Unsplit(split_tabs::SplitTabId split_id) override;
  void AddCollectionMapping(TabCollection* root_collection) override;
  void RemoveCollectionMapping(TabCollection* root_collection) override;
  const tree_tab::TreeTabNodeId* GetTreeTabNodeIdForGroup(
      tab_groups::TabGroupId group_id) const override;

 private:
  std::unique_ptr<BraveTabStripCollectionDelegate> delegate_;
};

}  // namespace tabs

#endif  // BRAVE_COMPONENTS_TABS_PUBLIC_BRAVE_TAB_STRIP_COLLECTION_H_
