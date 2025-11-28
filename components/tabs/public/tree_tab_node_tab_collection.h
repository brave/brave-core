// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TABS_PUBLIC_TREE_TAB_NODE_TAB_COLLECTION_H_
#define BRAVE_COMPONENTS_TABS_PUBLIC_TREE_TAB_NODE_TAB_COLLECTION_H_

#include <memory>

#include "base/callback_list.h"
#include "brave/components/tabs/public/tree_tab_node_id.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_interface.h"

namespace tabs {

class TreeTabNode;

// TreeTabNodeTabCollection is a specialized TabCollection that represents a
// node in the tree structure of tabs. It contains a current tab and can have
// child collections, such as other TreeTabNodeTabCollections,
// TabGroupTabCollections, SplitTabCollections.
class TreeTabNodeTabCollection : public tabs::TabCollection {
 public:
  // Builds the tree tabs structure starting from the root collection.
  // This will wrap all tabs in the tree with TreeTabNode
  static void BuildTreeTabs(
      TabCollection& root,
      base::RepeatingCallback<void(const TreeTabNode& node)> on_create);

  // Flattens the tree tabs structure by moving all tabs from TreeTabNodes
  // to their parent collections and removing the TreeTabNodes themselves.
  static void FlattenTreeTabs(
      TabCollection& root,
      base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> on_remove);

  TreeTabNodeTabCollection(const tree_tab::TreeTabNodeId& tree_tab_node_id,
                           std::unique_ptr<tabs::TabInterface> current_tab);
  ~TreeTabNodeTabCollection() override;

  TreeTabNode& node() { return *node_; }
  const TreeTabNode& node() const { return *node_; }

  // A tab that's associated with this TreeTabNode.
  const base::WeakPtr<tabs::TabInterface>& current_tab() const {
    return current_tab_;
  }

  // Returns the top-level ancestor TreeTabNode in the hierarchy.
  TreeTabNodeTabCollection* GetTopLevelAncestor();
  const TreeTabNodeTabCollection* GetTopLevelAncestor() const;

  // Returns the direct children of this TreeTabNode as a list of variants
  // containing either TabInterface* or TabCollection*.
  std::vector<std::variant<tabs::TabInterface*, TabCollection*>>
  GetTreeNodeChildren();

  // TabCollection:
  void OnReparentedImpl(TabCollection* old_parent,
                        TabCollection* new_parent) override;

 private:
  // Returns all TreeTabNodeTabCollections recursively from the given parent
  // collection.
  static void CollectTreeNodesRecursively(
      tabs::TabCollection& parent,
      std::vector<TreeTabNodeTabCollection*>& nodes);

  // Could be nullptr on closing the tab. Should be nulled out in order to avoid
  // dangling pointer issues.
  base::WeakPtr<tabs::TabInterface> current_tab_;

  // A class that represents metadata about the tree tab node.
  std::unique_ptr<TreeTabNode> node_;
};

}  // namespace tabs

#endif  // BRAVE_COMPONENTS_TABS_PUBLIC_TREE_TAB_NODE_TAB_COLLECTION_H_
