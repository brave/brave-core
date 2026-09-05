// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TABS_PUBLIC_BRAVE_TAB_STRIP_COLLECTION_DELEGATE_H_
#define BRAVE_COMPONENTS_TABS_PUBLIC_BRAVE_TAB_STRIP_COLLECTION_DELEGATE_H_

#include "base/types/pass_key.h"
#include "brave/components/tabs/public/brave_tab_strip_collection.h"
#include "components/split_tabs/split_tab_id.h"

namespace split_tabs {
class SplitTabVisualData;
}  // namespace split_tabs

namespace tree_tab {
class TreeTabNodeId;
}  // namespace tree_tab

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
      const TabCollection::TypeEnumSet retain_collection_types) = 0;

  // Inserts a tab collection (e.g. split or group) at a strip index. Used by
  // TabStripModel when re-attaching detached collections after drag-and-drop.
  virtual void InsertTabCollectionAt(
      std::unique_ptr<TabCollection> collection,
      int index,
      bool pinned,
      std::optional<tab_groups::TabGroupId> parent_group) {}

  // Handles CreateSplit when tabs are in different parent collections (e.g.
  // different tree nodes). Returns true if handled, false to use default path.
  virtual bool CreateSplit(split_tabs::SplitTabId split_id,
                           const std::vector<TabInterface*>& tabs,
                           split_tabs::SplitTabVisualData visual_data) const;
  // When handling (e.g. tree tabs), can no-op to keep tabs in split so
  // RemoveTabAtIndexRecursive sees parent SPLIT instead of TREE_NODE.
  // returns true if handled, false to use default path.
  virtual bool Unsplit(split_tabs::SplitTabId split_id);

  // Returns tab collection that should be added/removed from collection mapping
  // in TabStripCollection.
  virtual tabs::TabCollection* GetCollectionForMapping(
      tabs::TabCollection* root_collection);

  // Returns tree tab node id for a group.
  virtual const tree_tab::TreeTabNodeId* GetTreeTabNodeIdForGroup(
      tab_groups::TabGroupId group_id) const;

  // Called before a batch of tabs is detached together (e.g. for a
  // cross-window move). Hoists any child of a moving tab's tree node that is
  // NOT itself part of |moving_tabs| up to that tree node's parent, so each
  // moving tab's tree node afterwards contains only descendants that are also
  // moving. Must be called once, before any detaching begins.
  virtual void PrepareTreeTabNodesForBatchDetach(
      const std::vector<TabInterface*>& moving_tabs) {}

  // Returns true if |tab| is the topmost tab of a subtree that should be
  // detached as a single atomic TreeTabNodeTabCollection unit rather than via
  // the ordinary single-tab detach path.
  virtual bool ShouldDetachAsTreeSubtreeRoot(
      TabInterface* tab,
      const std::vector<TabInterface*>& moving_tabs);

  // Called just before |subtree_root| (and its descendant tree nodes) are
  // physically removed from the collection hierarchy for reinsertion
  // elsewhere (e.g. a different window).
  virtual void WillDetachTreeTabNodeSubtree(
      TreeTabNodeTabCollection& subtree_root) {}

  // Called just after |subtree_root| (and its descendant tree nodes) have
  // been physically inserted into the collection hierarchy.
  virtual void DidAttachTreeTabNodeSubtree(
      TreeTabNodeTabCollection& subtree_root) {}

 protected:
  base::PassKey<BraveTabStripCollectionDelegate> GetPassKey() const;

  // owner of this delegate.
  raw_ref<BraveTabStripCollection> collection_;
};

}  // namespace tabs

#endif  // BRAVE_COMPONENTS_TABS_PUBLIC_BRAVE_TAB_STRIP_COLLECTION_DELEGATE_H_
