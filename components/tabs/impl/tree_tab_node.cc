// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/tabs/public/tree_tab_node.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <variant>

#include "absl/functional/overload.h"
#include "base/containers/adapters.h"
#include "base/functional/bind.h"
#include "base/types/to_address.h"
#include "components/tabs/public/tab_interface.h"

// static
void TreeTabNode::BuildTreeTabs(TabCollection& root) {
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

    auto tree_node = std::make_unique<TreeTabNode>(
        tree_tab::TreeTabNodeId::GenerateNew(), std::move(owned_tab_interface));
    parent_collection->AddCollection(std::move(tree_node), index);
  }
}

void TreeTabNode::FlattenTreeTabs(TabCollection& root) {
  // Get all TreeTabNodes recursively from the root.
  std::vector<TreeTabNode*> all_tree_nodes;
  CollectTreeNodesRecursively(root, all_tree_nodes);

  // Process tree nodes in reverse order to handle children before parents.
  // i.e. we have all tree nodes in reverse post-order traversal.
  for (auto* tree_node : base::Reversed(all_tree_nodes)) {
    // Move all tabs from the tree node to its parent collection.

    // * We're going to remove tab tree nodes and insert all children of it to
    // where the tree node is.
    std::vector<std::variant<tabs::TabInterface*, TabCollection*>> children =
        tree_node->GetChildren();
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

    // * Remove the tree node itself.
    CHECK(tree_node->GetChildren().empty())
        << "Tree node should not have any children at this point.";
    auto tree_node_to_be_removed =
        parent_collection->MaybeRemoveCollection(tree_node);
  }
}

TreeTabNode::TreeTabNode(const tree_tab::TreeTabNodeId& tree_tab_node_id,
                         std::unique_ptr<tabs::TabInterface> current_tab)
    : TabCollection(TabCollection::Type::TREE_NODE,
                    /*supported_child_collections=*/
                    {TabCollection::Type::SPLIT, TabCollection::Type::GROUP,
                     TabCollection::Type::TREE_NODE},
                    /*supports_tabs=*/true),
      tree_tab_node_id_(tree_tab_node_id),
      current_tab_(base::to_address(current_tab)) {
  CHECK(!tree_tab_node_id.is_empty());
  CHECK(current_tab_);

  will_detach_tab_subscription_ = current_tab_->RegisterWillDetach(
      base::BindRepeating(&TreeTabNode::OnWillDetach, base::Unretained(this)));

  AddTab(std::move(current_tab), 0);
}

TreeTabNode::~TreeTabNode() = default;

TreeTabNode* TreeTabNode::GetTopLevelAncestor() {
  auto* parent = GetParentCollection();
  if (!parent || parent->type() != TabCollection::Type::TREE_NODE) {
    return this;
  }
  return static_cast<TreeTabNode*>(parent)->GetTopLevelAncestor();
}

const TreeTabNode* TreeTabNode::GetTopLevelAncestor() const {
  return const_cast<TreeTabNode*>(this)->GetTopLevelAncestor();
}

// static
void TreeTabNode::CollectTreeNodesRecursively(
    tabs::TabCollection& parent,
    std::vector<TreeTabNode*>& nodes) {
  for (auto& child : GetChildrenStatic(parent)) {
    if (std::holds_alternative<std::unique_ptr<tabs::TabInterface>>(child)) {
      continue;  // Skip tabs, we only want collections.
    }

    auto& collection = std::get<std::unique_ptr<tabs::TabCollection>>(child);
    if (collection->type() == TabCollection::Type::TREE_NODE) {
      auto* tree_node = static_cast<TreeTabNode*>(collection.get());
      nodes.push_back(tree_node);
    }

    // Recursively collect from all collections (including groups).
    CollectTreeNodesRecursively(*collection, nodes);
  }
}

std::vector<std::variant<tabs::TabInterface*, tabs::TabCollection*>>
TreeTabNode::GetChildren() {
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
        auto collection =
            std::get<std::unique_ptr<tabs::TabCollection>>(child).get();
        CHECK(collection->type() != TabCollection::Type::TREE_NODE)
            << "Tree node should not contain tree nodes as children at this "
               "point.";
        return collection;
      });

  return children;
}

void TreeTabNode::OnWillDetach(tabs::TabInterface* tab,
                               tabs::TabInterface::DetachReason detach_reason) {
  if (current_tab_ == tab) {
    current_tab_ = nullptr;
  }
}
