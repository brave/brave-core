// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/tabs/public/tree_tab_node.h"

#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "brave/components/tabs/public/tree_tab_node_id.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"

namespace tabs {

const TreeTabNode& TreeTabNode::GetEmptyTreeTabNode() {
  static base::NoDestructor<TreeTabNodeTabCollection>
      empty_tree_tab_node_tab_collection(tree_tab::TreeTabNodeId::GenerateNew(),
                                         nullptr, base::DoNothing(),
                                         base::DoNothing());
  return empty_tree_tab_node_tab_collection->node();
}

TreeTabNode::TreeTabNode(TreeTabNodeTabCollection& collection,
                         const tree_tab::TreeTabNodeId& id)
    : collection_(collection), id_(id) {}

int TreeTabNode::GetTreeHeight() const {
  return collection_->GetTopLevelAncestor()->node().height();
}

const TabInterface* TreeTabNode::GetTab() const {
  return collection_->current_tab() ? collection_->current_tab().get()
                                    : nullptr;
}

std::optional<tree_tab::TreeTabNodeId>
TreeTabNode::GetClosestCollapsedAncestorId() const {
  const TabCollection* current = collection_->GetParentCollection();
  while (current && current->type() == TabCollection::Type::TREE_NODE) {
    const auto* parent_tree =
        static_cast<const TreeTabNodeTabCollection*>(current);
    if (parent_tree->node().collapsed()) {
      return parent_tree->node().id();
    }
    current = current->GetParentCollection();
  }
  return std::nullopt;
}

void TreeTabNode::CollectDescendantIds(
    std::vector<tree_tab::TreeTabNodeId>& out) {
  for (const auto& child : collection_->GetTreeNodeChildren()) {
    if (std::holds_alternative<tabs::TabCollection*>(child)) {
      TabCollection* collection = std::get<tabs::TabCollection*>(child);
      if (collection->type() != TabCollection::Type::TREE_NODE) {
        continue;
      }
      auto* child_tree = static_cast<TreeTabNodeTabCollection*>(collection);
      const tree_tab::TreeTabNodeId& child_id = child_tree->node().id();
      out.push_back(child_id);
      child_tree->node().CollectDescendantIds(out);
    }
  }
}

void TreeTabNode::CollectUncollapseDescendantIds(
    std::vector<tree_tab::TreeTabNodeId>& out) {
  for (const auto& child : collection_->GetTreeNodeChildren()) {
    if (std::holds_alternative<tabs::TabCollection*>(child)) {
      TabCollection* collection = std::get<tabs::TabCollection*>(child);
      if (collection->type() != TabCollection::Type::TREE_NODE) {
        continue;
      }
      auto* child_tree = static_cast<TreeTabNodeTabCollection*>(collection);
      TreeTabNode& child_node = child_tree->node();
      out.push_back(child_node.id());
      if (!child_node.collapsed()) {
        child_node.CollectUncollapseDescendantIds(out);
      }
    }
  }
}

int TreeTabNode::CalculateLevelAndHeightRecursively(
    base::PassKey<TreeTabNodeTabCollection> pass_key) {
  return CalculateLevelAndHeightRecursivelyImpl();
}

void TreeTabNode::OnChildHeightChanged(
    base::PassKey<TreeTabNodeTabCollection> pass_key) {
  OnChildHeightChangedImpl();
}

int TreeTabNode::CalculateLevelAndHeightRecursivelyImpl() {
  auto* parent_collection = collection_->GetParentCollection();
  if (!parent_collection ||
      parent_collection->type() != TabCollection::Type::TREE_NODE) {
    // If there's no parent or the parent is not a tree node, this is the root.
    level_ = 0;
  } else {
    auto* parent_tree_node =
        static_cast<TreeTabNodeTabCollection*>(parent_collection);
    level_ = parent_tree_node->node().level_ + 1;
  }

  int max_height = std::numeric_limits<int>::min();
  for (const auto& child : collection_->GetTreeNodeChildren()) {
    if (std::holds_alternative<tabs::TabCollection*>(child)) {
      auto* collection = std::get<tabs::TabCollection*>(child);
      if (collection->type() != TabCollection::Type::TREE_NODE) {
        // If non-tree node child, height would be 1 for this node
        max_height = std::max(max_height, 1);
        continue;
      }

      max_height = std::max(max_height,
                            static_cast<TreeTabNodeTabCollection*>(collection)
                                    ->node()
                                    .CalculateLevelAndHeightRecursivelyImpl() +
                                1);
    }
  }

  height_ = (max_height == std::numeric_limits<int>::min()) ? 0 : max_height;

  return height_;
}

void TreeTabNode::OnChildHeightChangedImpl() {
  // Update height of this node.
  int max_height = std::numeric_limits<int>::min();
  for (const auto& child : collection_->GetTreeNodeChildren()) {
    if (std::holds_alternative<tabs::TabCollection*>(child)) {
      auto* collection = std::get<tabs::TabCollection*>(child);
      if (collection->type() != TabCollection::Type::TREE_NODE) {
        // If non-tree node child, height would be 1 for this node
        max_height = std::max(max_height, 1);
        continue;
      }

      max_height = std::max(
          max_height,
          static_cast<TreeTabNodeTabCollection*>(collection)->node().height_ +
              1);
    }
  }

  auto new_height =
      (max_height == std::numeric_limits<int>::min()) ? 0 : max_height;
  if (new_height == height_) {
    return;
  }

  height_ = new_height;

  if (auto* parent_collection = collection_->GetParentCollection();
      parent_collection &&
      parent_collection->type() == TabCollection::Type::TREE_NODE) {
    static_cast<TreeTabNodeTabCollection*>(parent_collection)
        ->node()
        .OnChildHeightChangedImpl();
  }
}

}  // namespace tabs
