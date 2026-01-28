// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TABS_BRAVE_TREE_TAB_STRIP_COLLECTION_DELEGATE_H_
#define BRAVE_BROWSER_UI_TABS_BRAVE_TREE_TAB_STRIP_COLLECTION_DELEGATE_H_

#include "base/types/expected.h"
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

  bool in_destruction_ = false;

  base::WeakPtr<TreeTabModel> tree_tab_model_;
};

#endif  // BRAVE_BROWSER_UI_TABS_BRAVE_TREE_TAB_STRIP_COLLECTION_DELEGATE_H_
