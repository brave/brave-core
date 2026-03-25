// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TABS_BRAVE_TREE_TAB_STRIP_COLLECTION_DELEGATE_H_
#define BRAVE_BROWSER_UI_TABS_BRAVE_TREE_TAB_STRIP_COLLECTION_DELEGATE_H_

#include <set>

#include "base/containers/flat_set.h"
#include "base/types/expected.h"
#include "brave/components/tabs/public/brave_tab_strip_collection_delegate.h"

class TreeTabModel;

namespace tabs {
class SplitTabCollection;
class TabGroupTabCollection;
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
  void MoveTabsRecursive(
      const std::vector<int>& tab_indices,
      size_t destination_index,
      std::optional<tab_groups::TabGroupId> new_group_id,
      bool new_pinned_state,
      const tabs::TabCollection::TypeEnumSet retain_collection_types) override;

  void InsertTabCollectionAt(
      std::unique_ptr<tabs::TabCollection> collection,
      int index,
      bool pinned,
      std::optional<tab_groups::TabGroupId> parent_group) override;

  bool CreateSplit(split_tabs::SplitTabId split_id,
                   const std::vector<tabs::TabInterface*>& tabs,
                   split_tabs::SplitTabVisualData visual_data) const override;
  bool Unsplit(split_tabs::SplitTabId split_id) override;
  tabs::TabCollection* GetCollectionForMapping(
      tabs::TabCollection* root_collection) override;
  const tree_tab::TreeTabNodeId* GetTreeTabNodeIdForGroup(
      tab_groups::TabGroupId group_id) const override;

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
      tabs::TreeTabNodeTabCollection* opener_collection,
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

  // Inserts a tree-node wrapper (callbacks + model registration) for detached
  // split/group collections; used by InsertTabCollectionAt.
  void InsertTreeNodeWrapperAt(
      std::unique_ptr<tabs::TreeTabNodeTabCollection> wrapper,
      int index,
      bool pinned,
      std::optional<tab_groups::TabGroupId> parent_group);
  void WrapCollectionInTreeNodeAndInsert(
      std::unique_ptr<tabs::SplitTabCollection> collection,
      int index,
      bool pinned,
      std::optional<tab_groups::TabGroupId> parent_group);
  void WrapCollectionInTreeNodeAndInsert(
      std::unique_ptr<tabs::TabGroupTabCollection> collection,
      int index,
      bool pinned,
      std::optional<tab_groups::TabGroupId> parent_group);

  // Unwraps the tree node and returns the split collection without the tree
  // node wrapper.
  std::unique_ptr<tabs::SplitTabCollection> UnwrapTreeNodeForCollection(
      tabs::SplitTabCollection* collection);

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

  // Groups where every tab in the group is in |moving_tabs|, so the move should
  // relocate the whole group collection. Empty when whole-group detection does
  // not apply (e.g. moving into a group or pinning).
  base::flat_set<tab_groups::TabGroupId> GetMovingGroups(
      const std::vector<tabs::TabInterface*>& moving_tabs,
      std::optional<tab_groups::TabGroupId> new_group_id,
      bool new_pinned_state,
      const tabs::TabCollection::TypeEnumSet& retain_collection_types) const;

  // Returns a vector of tabs or collections. Tabs in a moving group are
  // converted into a single collection. And Tabs in a split are converted into
  // a single collection.
  std::vector<std::variant<tabs::TabInterface*, tabs::TabCollection*>>
  CompactMovingTabs(
      const std::vector<tabs::TabInterface*>& moving_tabs,
      const tabs::TabCollection::TypeEnumSet& types_to_compact) const;

  // Returns the tab's parent collection in the strip; if the direct parent is
  // a split or group, returns the split/group's parent (e.g. group). This is
  // used to get the closest parent TreeNodeTabCollection
  tabs::TabCollection* GetParentCollectionSkippingTypes(
      tabs::TabInterface* tab,
      const tabs::TabCollection::TypeEnumSet& skipping_types) const;

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

  // Wraps a detached TabGroupTabCollection in a tree node and attaches it next
  // to the moving tabs' tree position (see MoveTabsIntoGroup).
  void AttachDetachedGroupCollection(
      const std::vector<tabs::TabInterface*>& moving_tabs,
      tab_groups::TabGroupId new_group_id) const;

  // Handles moving tabs into a group: get/attach group, unwrap from tree nodes,
  // add to group, clean up empty tree nodes.
  void MoveTabsIntoGroup(
      const std::vector<tabs::TabInterface*>& moving_tabs,
      size_t destination_index,
      tab_groups::TabGroupId new_group_id,
      const tabs::TabCollection::TypeEnumSet& retain_collection_types);

  // Called when a group has no tabs left: unwrap tree node, notify model,
  // detach group collection, remove wrapper.
  void OnGroupEmpty(tabs::TabGroupTabCollection* group_collection) const;

  // Detaches a single tab from its parent and returns it. Handles cases where
  // the parent is group or tree node.
  std::unique_ptr<tabs::TabInterface> DetachTabFromParent(
      tabs::TabInterface* tab) const;

  // Detaches a single split from its parent and returns it. Handles cases where
  // the parent is group or tree node.
  std::unique_ptr<tabs::TabCollection> DetachSplitFromParent(
      tabs::SplitTabCollection* collection);

  // Detaches a single tab from its group and returns it. If the group becomes
  // empty, removes the group collection and its tree node wrapper. The tab's
  // parent must be a TabGroupTabCollection.
  std::unique_ptr<tabs::TabInterface> DetachTabOutOfGroup(
      tabs::TabInterface* tab) const;

  // Handles moving tabs out of a group: remove from group, wrap each in a tree
  // node, insert at destination, remove empty group.
  void MoveTabsOutOfGroup(
      const std::vector<tabs::TabInterface*>& moving_tabs,
      size_t destination_index,
      bool new_pinned_state,
      const tabs::TabCollection::TypeEnumSet& retain_collection_types);

  // Returns true if the given tab is in a pinned collection.
  bool IsTabInPinnedCollection(tabs::TabInterface* tab) const;

  // Pins the given tabs and moves them to the destination index.
  void PinTabs(const std::vector<tabs::TabInterface*>& moving_tabs,
               size_t destination_index);

  // Pins a split whose both tabs are in `split_tab_set`, unwrapping a tree node
  // wrapper if needed. Returns early if the split is already pinned.
  void PinSplit(tabs::TabInterface* moving_tab,
                const std::set<tabs::TabInterface*>& split_tab_set,
                size_t destination_index);

  // Unpins a split from pinned: wraps the split in a tree node and attaches to
  // the unpinned collection at index 0 for a later MoveTabsRecursive pass.
  void UnpinSplit(tabs::TabInterface* moving_tab) const;

  // Unpins the given tabs and moves them to the destination index.
  void UnpinTabs(
      const std::vector<tabs::TabInterface*>& moving_tabs,
      size_t destination_index,
      std::optional<tab_groups::TabGroupId> new_group_id,
      const tabs::TabCollection::TypeEnumSet retain_collection_types);

  bool in_destruction_ = false;
  bool handling_pinned_state_inconsistency_ = false;

  base::WeakPtr<TreeTabModel> tree_tab_model_;
};

#endif  // BRAVE_BROWSER_UI_TABS_BRAVE_TREE_TAB_STRIP_COLLECTION_DELEGATE_H_
