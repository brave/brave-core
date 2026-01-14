// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TABS_TREE_TAB_MODEL_H_
#define BRAVE_BROWSER_UI_TABS_TREE_TAB_MODEL_H_

#include <map>

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

  // Returns the tree height value associated with the node identified by |id|.
  // The returned height is implementation-defined and is typically 0 if the
  // node does not exist or has no associated height information.
  int GetTreeHeight(const tree_tab::TreeTabNodeId& id) const;

  // Adds |node| to the model so it can be accessed via its TreeTabNodeId.
  // May trigger callbacks registered via RegisterAddTreeTabNodeCallback.
  void AddTreeTabNode(const tabs::TreeTabNode& node);

  // Removes the node identified by |id| from the model, if it exists.
  // May trigger callbacks registered via RegisterRemoveTreeTabNodeCallback.
  void RemoveTreeTabNode(const tree_tab::TreeTabNodeId& id);

  base::WeakPtr<TreeTabModel> GetWeakPtr();

  // Callback registration methods.
  base::CallbackListSubscription RegisterAddTreeTabNodeCallback(
      base::RepeatingCallback<void(const tabs::TreeTabNode&)> callback);
  base::CallbackListSubscription RegisterWillRemoveTreeTabNodeCallback(
      base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> callback);

 private:
  std::map<tree_tab::TreeTabNodeId, raw_ptr<const tabs::TreeTabNode>>
      tree_tab_nodes_;

  base::RepeatingCallbackList<void(const tabs::TreeTabNode&)>
      add_tree_tab_node_callback_list_;
  base::RepeatingCallbackList<void(const tree_tab::TreeTabNodeId&)>
      will_remove_tree_tab_node_callback_list_;

  base::WeakPtrFactory<TreeTabModel> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_TABS_TREE_TAB_MODEL_H_
