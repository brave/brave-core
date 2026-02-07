// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TABS_BRAVE_TREE_TAB_STRIP_COLLECTION_DELEGATE_H_
#define BRAVE_BROWSER_UI_TABS_BRAVE_TREE_TAB_STRIP_COLLECTION_DELEGATE_H_

#include "base/types/expected.h"
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
  // Tries to add the tab to the same tree as the opener. Returns base::ok(void)
  // if successful, base::unexpected(std::unique_ptr<tabs::TabInterface>) so
  // that the tab can be reused from the caller
  using AddTabResult =
      base::expected<void, std::unique_ptr<tabs::TabInterface>>;
  AddTabResult TryAddTabToSameTreeAsOpener(
      std::unique_ptr<tabs::TabInterface> tab,
      size_t index,
      tabs::TabInterface* opener) const;

  // Checks if opener and previous tab collections are in the same tree
  // hierarchy.
  bool AreInSameTreeHierarchy(
      tabs::TabCollection* opener_collection,
      tabs::TabCollection* previous_tab_collection) const;

  // Calculates the target index within the opener collection based on the
  // recursive index.
  std::optional<size_t> CalculateTargetIndexInOpenerCollection(
      tabs::TabCollection* opener_collection,
      size_t recursive_index) const;

  // Adds a tab as a tree node to the specified collection at the given index.
  void AddTabAsTreeNodeToCollection(std::unique_ptr<tabs::TabInterface> tab,
                                    tabs::TabCollection* target_collection,
                                    size_t target_index,
                                    size_t expected_recursive_index) const;

  // Adds a tab to the unpinned collection and wraps it with a tree node.
  void AddTabToUnpinnedCollectionAsTreeNode(
      size_t index,
      std::optional<tab_groups::TabGroupId> new_group_id,
      std::unique_ptr<tabs::TabInterface> tab) const;

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

  // Find the nearest attachable collection from the given tab collection, such
  // as TreeTabNodeTabCollection or UnpinnedTabCollection. It traverses up the
  // tree until it finds such collection.
  tabs::TabCollection* GetAttachableCollectionForTreeTabNode(
      tabs::TabCollection* tab_collection) const;

  bool in_destruction_ = false;

  base::WeakPtr<TreeTabModel> tree_tab_model_;
};

#endif  // BRAVE_BROWSER_UI_TABS_BRAVE_TREE_TAB_STRIP_COLLECTION_DELEGATE_H_
