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
class TreeTabNodeTabCollection;

// BraveTabStripCollection is a TabStripCollection that allows a delegate to
// override certain behaviors such as adding, removing, and moving tabs
// recursively. This is useful for implementing custom tab strip behaviors in
// Brave, while still leveraging the existing TabStripCollection logic.
//
// While the delegate handles the tab manipulation, observers are notified of
// the hierarchy with tree tab node collections unwrapped, i.e. as if their
// children were the children of their parents. The changes made through the
// delegate are made without notifications, and the difference of the
// unwrapped hierarchy is notified at the end of each operation instead.
class BraveTabStripCollection : public TabStripCollection {
 public:
  BraveTabStripCollection();
  ~BraveTabStripCollection() override;

  // Sets the delegate used to override tab strip collection behaviors.
  // Ownership of the |delegate| is transferred to the BraveTabStripCollection.
  void SetDelegate(std::unique_ptr<BraveTabStripCollectionDelegate> delegate);

  // Exposes APIs for delegate to override behaviors ---------------------------
  // Theses methods do what TabStripCollection does on behalf of the delegate,
  // without notifications. See the class comment.
  tabs::TabCollection* GetParentCollection(
      TabInterface* tab,
      base::PassKey<BraveTabStripCollectionDelegate> pass_key) const;
  const ChildrenVector& GetChildrenForDelegate(
      const TabCollection& collection,
      base::PassKey<BraveTabStripCollectionDelegate> pass_key) const;
  void AddTabRecursive(ScopedTab tab,
                       size_t index,
                       std::optional<tab_groups::TabGroupId> new_group_id,
                       bool new_pinned_state,
                       base::PassKey<BraveTabStripCollectionDelegate> pass_key);
  ScopedTab RemoveTabAtIndexRecursive(
      size_t index,
      base::PassKey<BraveTabStripCollectionDelegate> pass_key);
  void AddTabCollectionAtPosition(
      std::unique_ptr<TabCollection> collection,
      const TabCollection::Position& position,
      base::PassKey<BraveTabStripCollectionDelegate> pass_key);
  void InsertTabCollectionAt(
      std::unique_ptr<TabCollection> collection,
      int index,
      bool pinned,
      std::optional<tab_groups::TabGroupId> parent_group,
      base::PassKey<BraveTabStripCollectionDelegate> pass_key);
  std::unique_ptr<TabGroupTabCollection> PopDetachedGroupCollectionForDelegate(
      tab_groups::TabGroupId group_id,
      base::PassKey<BraveTabStripCollectionDelegate> pass_key);
  // Unlike the others, notifies the moves as TabStripCollection does. Only for
  // tabs in a group, where there are no tree tab node collections.
  void MoveTabsRecursiveForDelegate(
      const std::vector<int>& tab_indices,
      size_t destination_index,
      std::optional<tab_groups::TabGroupId> new_group_id,
      bool new_pinned_state,
      const TabCollection::TypeEnumSet retain_collection_types,
      base::PassKey<BraveTabStripCollectionDelegate> pass_key);

  // Moves |collection| to the |index| of |new_parent| and notifies observers.
  void MoveCollectionAndNotify(TabCollection* collection,
                               TabCollection* new_parent,
                               size_t index);

  // TabStripCollection:
  void AddTabRecursive(ScopedTab tab,
                       size_t index,
                       std::optional<tab_groups::TabGroupId> new_group_id,
                       bool new_pinned_state,
                       TabInterface* opener) override;
  void InsertTabCollectionAt(
      std::unique_ptr<TabCollection> collection,
      int index,
      bool pinned,
      std::optional<tab_groups::TabGroupId> parent_group) override;
  void MoveTabsRecursive(
      const std::vector<int>& tab_indices,
      size_t destination_index,
      std::optional<tab_groups::TabGroupId> new_group_id,
      bool new_pinned_state,
      const TabCollection::TypeEnumSet retain_collection_types) override;
  ScopedTab RemoveTabAtIndexRecursive(size_t index) override;
  void CreateSplit(split_tabs::SplitTabId split_id,
                   const std::vector<TabInterface*>& tabs,
                   split_tabs::SplitTabVisualData visual_data) override;
  void Unsplit(split_tabs::SplitTabId split_id) override;
  void AddCollectionMapping(TabCollection* root_collection) override;
  void RemoveCollectionMapping(TabCollection* root_collection) override;
  const tree_tab::TreeTabNodeId* GetTreeTabNodeIdForGroup(
      tab_groups::TabGroupId group_id) const override;
  void PrepareTreeTabNodesForBatchDetach(
      const std::vector<TabInterface*>& moving_tabs) override;
  bool ShouldDetachAsTreeSubtreeRoot(
      TabInterface* tab,
      const std::vector<TabInterface*>& moving_tabs) override;
  void WillDetachTreeTabNodeSubtree(
      TreeTabNodeTabCollection& subtree_root) override;
  void DidAttachTreeTabNodeSubtree(
      TreeTabNodeTabCollection& subtree_root) override;
  void InsertDetachedTreeTabNode(std::unique_ptr<TabCollection> collection,
                                 int index) override;
  std::unique_ptr<TabCollection> RemoveTabCollection(
      TabCollection* collection) override;

 private:
  class ScopedUnwrappedNotifications;
  struct UnwrappedHierarchy;

  bool ShouldDelegate() const;

  // TabStripCollection::AddTabCollectionImpl() and RemoveTabCollectionImpl()
  // without notifications.
  void AddTabCollectionSilently(std::unique_ptr<TabCollection> collection,
                                const TabCollection::Position& position);
  std::unique_ptr<TabCollection> RemoveTabCollectionSilently(
      TabCollection* collection);

  // Notifies observers of the changes of the unwrapped hierarchy since
  // |unwrapped_hierarchy_snapshot_|, which is updated to the current one.
  void NotifyUnwrappedChanges();
  void NotifyRemovedNodes(const UnwrappedHierarchy& current,
                          const TabCollectionHandle& parent);
  void NotifyPlacedNodes(const UnwrappedHierarchy& current,
                         const TabCollectionHandle& parent);

  std::unique_ptr<BraveTabStripCollectionDelegate> delegate_;

  // The unwrapped hierarchy that observers know of. Set while the delegate is
  // handling an operation. See ScopedUnwrappedNotifications.
  std::unique_ptr<UnwrappedHierarchy> unwrapped_hierarchy_snapshot_;
  int unwrapped_notifications_depth_ = 0;
};

}  // namespace tabs

#endif  // BRAVE_COMPONENTS_TABS_PUBLIC_BRAVE_TAB_STRIP_COLLECTION_H_
