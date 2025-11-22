// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/tabs/public/brave_tab_strip_collection_delegate.h"

class TreeTabModel;

namespace tabs {
class TreeTabNodeTabCollection;
}  // namespace tabs

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
  // Move all direct children of TreeTabNodeTabCollection to its parent at the
  // position of the TreeTabNodeTabCollection. The "current tab" of the given
  // tree tab node collection will be skipped.
  void MoveChildrenOfTreeTabNodeToParent(
      tabs::TreeTabNodeTabCollection* tree_tab_node_collection) const;

  // Move all direct children of TreeTabNodeTabCollection except the
  // "current_tab" to the target TreeTabNodeTabCollection.
  void MoveChildrenOfTreeTabNodeToNode(
      tabs::TreeTabNodeTabCollection* tree_tab_node_collection,
      tabs::TabCollection* target_collection,
      size_t target_index) const;

  // Returns parent TreeTabNodeTabCollection of the given tab. In case the tab's
  // direct parent is not a TreeTabNodeTabCollection, e.g. GROUP, SPLIT, it goes
  // up the tree until it finds a TreeTabNodeTabCollection or reaches the
  // unpinned collection.
  tabs::TreeTabNodeTabCollection* GetParentTreeNodeCollectionOfTab(
      tabs::TabInterface* tab) const;

  // Find the nearest attachable collection from the given tab collection, such as
  // TreeTabNodeTabCollection or UnpinnedTabCollection. It traverses up the tree
  // until it finds such collection.
  tabs::TabCollection* GetAttachableCollectionForTreeTabNode(
      tabs::TabCollection* tab_collection) const;

  bool in_desturction_ = false;

  base::WeakPtr<TreeTabModel> tree_tab_model_;
};
