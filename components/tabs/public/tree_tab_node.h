// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef TREE_TAB_NODE_H_
#define TREE_TAB_NODE_H_

#include "base/memory/raw_ref.h"
#include "base/types/pass_key.h"
#include "brave/components/tabs/public/tree_tab_node_id.h"

namespace tabs {

class TabInterface;
class TreeTabNodeTabCollection;

// A class that represents metadata about a tree tab node.
class TreeTabNode {
 public:
  TreeTabNode(TreeTabNodeTabCollection& collection,
              const tree_tab::TreeTabNodeId& id);
  TreeTabNode(const TreeTabNode&) = delete;
  TreeTabNode& operator=(const TreeTabNode&) = delete;
  ~TreeTabNode() = default;

  const tree_tab::TreeTabNodeId& id() const { return id_; }

  int height() const { return height_; }
  int level() const { return level_; }

  void set_collapsed(bool collapsed) { collapsed_ = collapsed; }
  bool collapsed() const { return collapsed_; }

  // Returns the height of the tree that this node belongs to.
  int GetTreeHeight() const;

  // Returns the tab associated with this tree tab node.
  const TabInterface* GetTab() const;

  // Exposes the calculation of level and height to TreeTabNodeTabCollection.
  int CalculateLevelAndHeightRecursively(
      base::PassKey<TreeTabNodeTabCollection> pass_key);
  void OnChildHeightChanged(base::PassKey<TreeTabNodeTabCollection> pass_key);

 private:
  // Recalculates the level and height of this node and its children recursively
  // in the tree. This returns the deepest height of the subtree rooted at this
  // node.
  int CalculateLevelAndHeightRecursivelyImpl();

  // Called when child node's height changes to update this node's height.
  void OnChildHeightChangedImpl();

  // Owner of this TreeNode.
  base::raw_ref<TreeTabNodeTabCollection> collection_;

  // id of this tree tab node.
  tree_tab::TreeTabNodeId id_;

  // The level of this node in the tree. Root is level 0, its children are
  // level 1, and so on.
  int level_ = 0;

  // The height of the subtree rooted at this node. A leaf node has height 0.
  // This is used for calculating the level of nodes efficiently.
  int height_ = 0;

  // When this is true, the child tabs under this tree node are hidden.
  bool collapsed_ = false;
};

}  // namespace tabs

#endif  // TREE_TAB_NODE_H_
