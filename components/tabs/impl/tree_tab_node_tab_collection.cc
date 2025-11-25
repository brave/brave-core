// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <variant>

#include "absl/functional/overload.h"
#include "base/check_op.h"
#include "base/containers/adapters.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/types/to_address.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "components/tabs/public/tab_interface.h"

namespace tabs {

// static
void TreeTabNodeTabCollection::BuildTreeTabs(
    TabCollection& root,
    base::RepeatingCallback<void(const TreeTabNode& node)> on_create,
    base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> on_remove) {
  auto tabs = root.GetTabsRecursive();

  while (!tabs.empty()) {
    auto* tab_interface = tabs.back();
    tabs.pop_back();

    auto* parent_collection =
        tab_interface->GetParentCollection(GetPassKeyStatic());
    CHECK(parent_collection) << "Tab should always have a parent collection.";
    CHECK_NE(parent_collection->type(), TabCollection::Type::PINNED)
        << "Pinned tabs should not be wrapped in TreeTabNode.";
    int index = *parent_collection->GetIndexOfTab(tab_interface);
    auto owned_tab_interface = parent_collection->MaybeRemoveTab(tab_interface);
    CHECK_EQ(tab_interface, owned_tab_interface.get());

    auto tree_node = std::make_unique<TreeTabNodeTabCollection>(
        tree_tab::TreeTabNodeId::GenerateNew(), std::move(owned_tab_interface),
        on_remove);
    on_create.Run(tree_node->node());
    parent_collection->AddCollection(std::move(tree_node), index);
  }
}

void TreeTabNodeTabCollection::FlattenTreeTabs(TabCollection& root) {
  // Get all TreeTabNodes recursively from the root.
  std::vector<TreeTabNodeTabCollection*> all_tree_nodes;
  CollectTreeNodesRecursively(root, all_tree_nodes);

  // Process tree nodes in reverse order to handle children before parents.
  // i.e. we have all tree nodes in reverse post-order traversal.
  for (auto* tree_node : base::Reversed(all_tree_nodes)) {
    // Move all tabs from the tree node to its parent collection.

    // * We're going to remove tab tree nodes and insert all children of it to
    // where the tree node is.
    std::vector<std::variant<tabs::TabInterface*, TabCollection*>> children =
        tree_node->GetTreeNodeChildren();
    auto* parent_collection = tree_node->GetParentCollection();
    CHECK(parent_collection)
        << "Tree node should always have a parent collection.";
    auto index = parent_collection->GetIndexOfCollection(tree_node).value();

    // * Move tabs and collection to the parent collection at the same index as
    // the tree node.
    for (const auto& child : base::Reversed(children)) {
      std::visit(absl::Overload{
                     [&](tabs::TabInterface* tab) {
                       parent_collection->AddTab(tree_node->MaybeRemoveTab(tab),
                                                 index);
                     },
                     [&](tabs::TabCollection* collection) {
                       parent_collection->AddCollection(
                           tree_node->MaybeRemoveCollection(collection), index);
                     }},
                 child);
    }

    // * Remove the tree node itself. The on_remove callback will be invoked
    // automatically in the TreeTabNodeTabCollection destructor.
    CHECK(tree_node->GetTreeNodeChildren().empty())
        << "Tree node should not have any children at this point.";
    auto tree_node_to_be_removed =
        parent_collection->MaybeRemoveCollection(tree_node);
  }
}

TreeTabNodeTabCollection::TreeTabNodeTabCollection(
    const tree_tab::TreeTabNodeId& tree_tab_node_id,
    std::unique_ptr<tabs::TabInterface> current_tab,
    base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> on_remove)
    : TabCollection(TabCollection::Type::TREE_NODE,
                    /*supported_child_collections=*/
                    {TabCollection::Type::SPLIT, TabCollection::Type::GROUP,
                     TabCollection::Type::TREE_NODE},
                    /*supports_tabs=*/true),
      current_tab_(current_tab->GetWeakPtr()),
      on_remove_(std::move(on_remove)),
      node_(std::make_unique<TreeTabNode>(*this, tree_tab_node_id)) {
  CHECK(!tree_tab_node_id.is_empty());
  CHECK(current_tab_);

  AddTab(std::move(current_tab), 0);
}

TreeTabNodeTabCollection::~TreeTabNodeTabCollection() {
  if (on_remove_) {
    on_remove_.Run(node_->id());
  }
}

TreeTabNodeTabCollection* TreeTabNodeTabCollection::GetTopLevelAncestor() {
  auto* parent = GetParentCollection();
  if (!parent || parent->type() != TabCollection::Type::TREE_NODE) {
    return this;
  }
  return static_cast<TreeTabNodeTabCollection*>(parent)->GetTopLevelAncestor();
}

const TreeTabNodeTabCollection* TreeTabNodeTabCollection::GetTopLevelAncestor()
    const {
  return const_cast<TreeTabNodeTabCollection*>(this)->GetTopLevelAncestor();
}

// static
void TreeTabNodeTabCollection::CollectTreeNodesRecursively(
    tabs::TabCollection& parent,
    std::vector<TreeTabNodeTabCollection*>& nodes) {
  for (auto& child : GetChildrenStatic(parent)) {
    if (std::holds_alternative<std::unique_ptr<tabs::TabInterface>>(child)) {
      continue;  // Skip tabs, we only want collections.
    }

    auto& collection = std::get<std::unique_ptr<tabs::TabCollection>>(child);
    if (collection->type() == TabCollection::Type::TREE_NODE) {
      auto* tree_node =
          static_cast<TreeTabNodeTabCollection*>(collection.get());
      nodes.push_back(tree_node);
    }

    // Recursively collect from all collections (including groups).
    CollectTreeNodesRecursively(*collection, nodes);
  }
}

std::vector<std::variant<tabs::TabInterface*, tabs::TabCollection*>>
TreeTabNodeTabCollection::GetTreeNodeChildren() {
  const auto& unique_children = GetChildrenStatic(*this);
  std::vector<std::variant<tabs::TabInterface*, TabCollection*>> children;
  children.reserve(unique_children.size());

  // Transforms unique_ptrs to raw pointers for the children.
  std::ranges::transform(
      unique_children, std::back_inserter(children),
      [](const auto& child)
          -> std::variant<tabs::TabInterface*, TabCollection*> {
        if (std::holds_alternative<std::unique_ptr<tabs::TabInterface>>(
                child)) {
          return std::get<std::unique_ptr<tabs::TabInterface>>(child).get();
        }
        return std::get<std::unique_ptr<tabs::TabCollection>>(child).get();
      });

  return children;
}

}  // namespace tabs
