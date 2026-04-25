// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TABS_PUBLIC_TREE_TAB_NODE_TAB_COLLECTION_H_
#define BRAVE_COMPONENTS_TABS_PUBLIC_TREE_TAB_NODE_TAB_COLLECTION_H_

#include <memory>
#include <optional>
#include <variant>

#include "base/callback_list.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/tabs/public/tree_tab_node_id.h"
#include "components/tabs/public/split_tab_collection.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_group_tab_collection.h"
#include "components/tabs/public/tab_interface.h"

namespace tabs {

class TreeTabNode;

// TreeTabNodeTabCollection is a specialized TabCollection that represents a
// node in the tree structure of tabs. It has a current value (a tab,
// TabGroupTabCollection, or SplitTabCollection) and can have child collections,
// such as other TreeTabNodeTabCollections, TabGroupTabCollections,
// SplitTabCollections.
class TreeTabNodeTabCollection : public tabs::TabCollection {
 public:
  // The value associated with this tree node: either a tab, a tab group
  // collection, or a split tab collection.
  using CurrentValueVariant = std::variant<base::WeakPtr<tabs::TabInterface>,
                                           raw_ptr<tabs::TabGroupTabCollection>,
                                           raw_ptr<tabs::SplitTabCollection>>;

  enum CurrentValueType {
    kTab,
    kSplit,
    kGroup,
  };

  // Builds the tree tabs structure starting from the root collection.
  // This will wrap all tabs in the tree with TreeTabNode. |on_create| is called
  // whenever a new TreeTabNode is created. |on_remove| is called when a
  // TreeTabNode is destroyed. |on_move| is called when a node is reparented
  // (moved to another parent).
  static void BuildTreeTabs(
      TabCollection& root,
      base::RepeatingCallback<void(TreeTabNode& node)> on_create,
      base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> on_remove,
      base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> on_move);

  // Flattens the tree tabs structure by moving all tabs from TreeTabNodes
  // to their parent collections and removing the TreeTabNodes themselves.
  static void FlattenTreeTabs(TabCollection& root);

  TreeTabNodeTabCollection(
      const tree_tab::TreeTabNodeId& tree_tab_node_id,
      std::unique_ptr<tabs::TabInterface> current_tab,
      base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> on_remove,
      base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> on_move);

  // Overload that wraps an existing TabCollection (e.g. SplitTabCollection)
  // as the current value of this tree node. Used when creating a split from
  // tabs in different tree nodes.
  TreeTabNodeTabCollection(
      const tree_tab::TreeTabNodeId& tree_tab_node_id,
      std::unique_ptr<tabs::SplitTabCollection> current_collection,
      base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> on_remove,
      base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> on_move);

  // Overload that wraps a TabGroupTabCollection as the current value of this
  // tree node. Used when creating a tab group in tree tab mode.
  TreeTabNodeTabCollection(
      const tree_tab::TreeTabNodeId& tree_tab_node_id,
      std::unique_ptr<tabs::TabGroupTabCollection> current_collection,
      base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> on_remove,
      base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> on_move);

  ~TreeTabNodeTabCollection() override;

  TreeTabNode& node() { return *node_; }
  const TreeTabNode& node() const { return *node_; }

  // The value (tab, tab group collection, or split tab collection) associated
  // with this TreeTabNode. Empty for the empty tree node (e.g.
  // GetEmptyTreeTabNode).
  const std::optional<CurrentValueVariant>& current_value() const {
    return current_value_;
  }

  // Helper method to get the current tab when the current value type is kTab.
  tabs::TabInterface* GetCurrentTab() const;

  // Helper method to get the current collection when the current value type is
  // kSplit or kGroup.
  tabs::TabCollection* GetCurrentCollection() const;

  // Returns the top-level ancestor TreeTabNodeTabCollection in the hierarchy.
  TreeTabNodeTabCollection* GetTopLevelAncestor();
  const TreeTabNodeTabCollection* GetTopLevelAncestor() const;

  // Returns the direct children of this TreeTabNode as a list of variants
  // containing either TabInterface* or TabCollection*.
  std::vector<std::variant<tabs::TabInterface*, TabCollection*>>
  GetTreeNodeChildren();

  // TabCollection:
  void OnReparented(TabCollection* new_parent) override;
  [[nodiscard]] std::unique_ptr<TabCollection> MaybeRemoveCollection(
      TabCollection* collection) override;

  CurrentValueType current_value_type() const { return current_value_type_; }

 private:
  // Returns all TreeTabNodeTabCollections recursively from the given parent
  // collection.
  static void CollectTreeNodesRecursively(
      tabs::TabCollection& parent,
      std::vector<TreeTabNodeTabCollection*>& nodes);

  CurrentValueType current_value_type_;

  // The current value: tab (WeakPtr), TabGroupTabCollection, or
  // SplitTabCollection. Empty for the empty tree node.
  std::optional<CurrentValueVariant> current_value_;

  // Callback invoked when this TreeTabNode is destroyed.
  base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> on_remove_;

  // Callback invoked when this TreeTabNode is reparented (moved to another
  // parent).
  base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> on_move_;

  // A class that represents metadata about the tree tab node.
  std::unique_ptr<TreeTabNode> node_;
};

}  // namespace tabs

#endif  // BRAVE_COMPONENTS_TABS_PUBLIC_TREE_TAB_NODE_TAB_COLLECTION_H_
