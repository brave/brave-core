// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TABS_TREE_TAB_MODEL_H_
#define BRAVE_BROWSER_UI_TABS_TREE_TAB_MODEL_H_

#include <map>
#include <set>

#include "base/callback_list.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/tabs/public/tree_tab_node.h"

// A model that contains TreeTabNodes. TreeTabNode provides metadata for tabs
// related to tree tab functionality, such as level, height, and collapsed
// state. All UI components except TabStripModel should only access TreeTabNodes
// via this model and should not access TreeTabNodeTabCollection directly.
class TreeTabModel {
 public:
  TreeTabModel();
  ~TreeTabModel();

  // Returns the TreeTabNode associated with the given |id|.
  // Returns nullptr if no node with the specified |id| exists in the model.
  const tabs::TreeTabNode* GetNode(const tree_tab::TreeTabNodeId& id) const;
  tabs::TreeTabNode* GetNode(const tree_tab::TreeTabNodeId& id);

  // Returns the tree height value associated with the node identified by |id|.
  // The returned height is implementation-defined and is typically 0 if the
  // node does not exist or has no associated height information.
  int GetTreeHeight(const tree_tab::TreeTabNodeId& id) const;

  // Sets the collapsed state of the node identified by |id| to |collapsed|.
  // Updates the internal cache for DoesBelongToCollapsedNode. No-op if no node
  // exists for |id|.
  void SetCollapsed(const tree_tab::TreeTabNodeId& id, bool collapsed);

  // Returns true if the node identified by |id| is under a collapsed ancestor
  // (and thus its tab should be hidden). O(1) from cache.
  bool DoesBelongToCollapsedNode(const tree_tab::TreeTabNodeId& id) const;

  // Adds |node| to the model so it can be accessed via its TreeTabNodeId.
  // May trigger callbacks registered via RegisterAddTreeTabNodeCallback.
  void AddTreeTabNode(tabs::TreeTabNode& node);

  // Removes the node identified by |id| from the model, if it exists.
  // May trigger callbacks registered via RegisterRemoveTreeTabNodeCallback.
  void RemoveTreeTabNode(const tree_tab::TreeTabNodeId& id);

  // Called when a node is reparented (moved to another parent). Updates the
  // collapsed cache for that node and its descendants.
  void OnTreeTabNodeMoved(const tree_tab::TreeTabNodeId& id);

  // Returns the closest collapsed ancestor of the node identified by |id|.
  // Returns nullptr if no collapsed ancestor exists.
  const tree_tab::TreeTabNodeId* GetClosestCollapsedAncestor(
      const tree_tab::TreeTabNodeId& id) const;

  base::WeakPtr<TreeTabModel> GetWeakPtr();

  // Callback registration methods.
  base::CallbackListSubscription RegisterAddTreeTabNodeCallback(
      base::RepeatingCallback<void(const tabs::TreeTabNode&)> callback);
  base::CallbackListSubscription RegisterWillRemoveTreeTabNodeCallback(
      base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> callback);

 private:
  std::map<tree_tab::TreeTabNodeId, raw_ptr<tabs::TreeTabNode>> tree_tab_nodes_;

  // For each node id that is under a collapsed ancestor, maps to the id of that
  // closest collapsed ancestor. Used for O(1) DoesBelongToCollapsedNode.
  std::map<tree_tab::TreeTabNodeId, tree_tab::TreeTabNodeId>
      closest_collapsed_ancestor_;

  // Reverse index: ancestor_id -> set of node_ids that have it as closest
  // collapsed ancestor. Avoids O(n) scan when uncollapsing or removing a node.
  std::map<tree_tab::TreeTabNodeId, std::set<tree_tab::TreeTabNodeId>>
      descendant_ids_by_collapsed_ancestor_;

  base::RepeatingCallbackList<void(const tabs::TreeTabNode&)>
      add_tree_tab_node_callback_list_;
  base::RepeatingCallbackList<void(const tree_tab::TreeTabNodeId&)>
      will_remove_tree_tab_node_callback_list_;

  base::WeakPtrFactory<TreeTabModel> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_TABS_TREE_TAB_MODEL_H_
