// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TABS_PUBLIC_TREE_TAB_NODE_H_
#define BRAVE_COMPONENTS_TABS_PUBLIC_TREE_TAB_NODE_H_

#include <optional>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/types/pass_key.h"
#include "brave/components/tabs/public/tree_tab_node_id.h"

namespace tabs {

class TabInterface;
class TreeTabNodeTabCollection;

// A class that represents metadata about a tree tab node.
class TreeTabNode {
 public:
  // Returns the empty tree tab node. This is used when a tree tab node is not
  // associated with a tab in tests.
  static const TreeTabNode& GetEmptyTreeTabNode();

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

  // Returns the height of the entire tree that this node belongs to. i.e. this
  // method will traverse up to the root node and return its height.
  int GetTreeHeight() const;

  // Returns the tab associated with this tree tab node, or nullptr if this
  // node does not currently have an associated tab
  const TabInterface* GetTab() const;

  // Returns the id of the closest ancestor that is collapsed, or nullopt if
  // no ancestor is collapsed.
  std::optional<tree_tab::TreeTabNodeId> GetClosestCollapsedAncestorId() const;

  // Appends the ids of all descendant tree tab nodes to |out|. Requires
  // non-const because it uses GetTreeNodeChildren() on the collection.
  void CollectDescendantIds(std::vector<tree_tab::TreeTabNodeId>& out);

  // Appends the ids of descendant tree tab nodes to |out| but does not recurse
  // into collapsed nodes. Use when assigning this node as closest collapsed
  // ancestor so nodes under a closer collapsed ancestor are not overwritten.
  void CollectUncollapseDescendantIds(
      std::vector<tree_tab::TreeTabNodeId>& out);

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

#endif  // BRAVE_COMPONENTS_TABS_PUBLIC_TREE_TAB_NODE_H_
