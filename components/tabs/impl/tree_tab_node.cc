// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/tabs/public/tree_tab_node.h"

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

void TreeTabNode::OnReparentedImpl(TabCollection* old_parent,
                                   TabCollection* new_parent) {
  CalculateLevelAndHeightRecursively();

  CHECK_NE(old_parent, new_parent);

  if (new_parent) {
    if (new_parent->type() == TabCollection::Type::TREE_NODE) {
      static_cast<TreeTabNode*>(new_parent)->OnChildHeightChanged();
    }
  } else {
    if (old_parent && old_parent->type() == TabCollection::Type::TREE_NODE) {
      static_cast<TreeTabNode*>(old_parent)->OnChildHeightChanged();
    }
  }
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
        return std::get<std::unique_ptr<tabs::TabCollection>>(child).get();
      });

  return children;
}

void TreeTabNode::OnWillDetach(tabs::TabInterface* tab,
                               tabs::TabInterface::DetachReason detach_reason) {
  if (current_tab_ == tab) {
    current_tab_ = nullptr;
  }
}

int TreeTabNode::CalculateLevelAndHeightRecursively() {
  auto* parent_collection = GetParentCollection();
  if (!parent_collection ||
      parent_collection->type() != TabCollection::Type::TREE_NODE) {
    // If there's no parent or the parent is not a tree node, this is the root.
    LOG(ERROR) << "TreeTabNode level == 0 " << " parent? "
               << (parent_collection != nullptr) << " type? "
               << (parent_collection
                       ? static_cast<int>(parent_collection->type())
                       : -1);
    level_ = 0;
  } else {
    auto* parent_tree_node = static_cast<TreeTabNode*>(parent_collection);
    level_ = parent_tree_node->level_ + 1;
    LOG(ERROR) << "TreeTabNode level updated: " << level_;
  }

  int max_height = std::numeric_limits<int>::min();
  for (const auto& child : GetChildren()) {
    if (std::holds_alternative<tabs::TabCollection*>(child)) {
      auto* collection = std::get<tabs::TabCollection*>(child);
      if (collection->type() != TabCollection::Type::TREE_NODE) {
        continue;
      }

      CHECK(collection->GetParentCollection() == this);

      max_height =
          std::max(max_height, static_cast<TreeTabNode*>(collection)
                                       ->CalculateLevelAndHeightRecursively() +
                                   1);
    }
  }

  height_ = (max_height == std::numeric_limits<int>::min()) ? 0 : max_height;

  return height_;
}

void TreeTabNode::OnChildHeightChanged() {
  // Update height of this node.
  int max_height = std::numeric_limits<int>::min();
  for (const auto& child : GetChildren()) {
    if (std::holds_alternative<tabs::TabCollection*>(child)) {
      auto* collection = std::get<tabs::TabCollection*>(child);
      if (collection->type() != TabCollection::Type::TREE_NODE) {
        continue;
      }

      max_height = std::max(max_height,
                            static_cast<TreeTabNode*>(collection)->height_ + 1);
    }
  }

  auto new_height =
      (max_height == std::numeric_limits<int>::min()) ? 0 : max_height;
  if (new_height == height_) {
    return;
  }

  height_ = new_height;

  if (auto* parent_collection = GetParentCollection();
      parent_collection &&
      parent_collection->type() == TabCollection::Type::TREE_NODE) {
    static_cast<TreeTabNode*>(parent_collection)->OnChildHeightChanged();
  }
}
