// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TABS_PUBLIC_TREE_TAB_NODE_H_
#define BRAVE_COMPONENTS_TABS_PUBLIC_TREE_TAB_NODE_H_

#include <memory>

#include "brave/components/tabs/public/tree_tab_node_id.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_interface.h"

// TreeTabNode is a specialized TabCollection that represents a node in the
// tree structure of tabs. It contains a current tab and can have child
// collections, such as other TreeTabNodes, TabGroupTabCollections,
// SplitTabCollections.
class TreeTabNode : public tabs::TabCollection {
 public:
  // Builds the tree tabs structure starting from the root collection.
  // This will wrap all tabs in the tree with TreeTabNode
  static void BuildTreeTabs(TabCollection& root);

  // Flattens the tree tabs structure by moving all tabs from TreeTabNodes
  // to their parent collections and removing the TreeTabNodes themselves.
  static void FlattenTreeTabs(TabCollection& root);

  TreeTabNode(const tree_tab::TreeTabNodeId& tree_tab_node_id,
              std::unique_ptr<tabs::TabInterface> current_tab);
  ~TreeTabNode() override;

  const tree_tab::TreeTabNodeId& tree_tab_node_id() const {
    return tree_tab_node_id_;
  }

  const tabs::TabInterface* current_tab() const { return current_tab_; }
  tabs::TabInterface* current_tab() { return current_tab_; }

  int height() const { return height_; }
  int level() const { return level_; }

  // Returns the top-level ancestor TreeTabNode in the hierarchy.
  TreeTabNode* GetTopLevelAncestor();
  const TreeTabNode* GetTopLevelAncestor() const;

  // TabCollection:
  void OnReparentedImpl(TabCollection* old_parent,
                        TabCollection* new_parent) override;

 private:
  // Returns all TreeTabNodes recursively from the given parent collection.
  static void CollectTreeNodesRecursively(tabs::TabCollection& parent,
                                          std::vector<TreeTabNode*>& nodes);

  // Callback for when the current tab is about to be detached.
  // This is used to ensure that the current tab is properly handled when the
  // TreeTabNode is removed or the tab is closed.
  void OnWillDetach(tabs::TabInterface*,
                    tabs::TabInterface::DetachReason tab_detach_reason);

  // Recalculates the level and height of this node and its children recursively
  // in the tree. This returns the deepest height of the subtree rooted at this
  // node.
  int CalculateLevelAndHeightRecursively();

  // Called when child node's height changes to update this node's height.
  void OnChildHeightChanged();

  // Returns the direct children of this TreeTabNode as a list of variants
  // containing either TabInterface* or TabCollection*.
  std::vector<std::variant<tabs::TabInterface*, TabCollection*>> GetChildren();

  tree_tab::TreeTabNodeId tree_tab_node_id_;

  // Could be nullptr on closing the tab. Should be nulled out in order to avoid
  // dangling pointer issues.
  raw_ptr<tabs::TabInterface> current_tab_;

  // The level of this node in the tree. Root is level 0, its children are
  // level 1, and so on.
  int level_ = 0;

  // The height of the subtree rooted at this node. A leaf node has height 0.
  // This is used for calculating the level of nodes efficiently.
  int height_ = 0;

  base::CallbackListSubscription will_detach_tab_subscription_;
};

#endif  // BRAVE_COMPONENTS_TABS_PUBLIC_TREE_TAB_NODE_H_
