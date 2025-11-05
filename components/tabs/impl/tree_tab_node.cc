// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/tabs/public/tree_tab_node.h"

#include "base/logging.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"

namespace tabs {

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
    // LOG(ERROR) << "TreeTabNode level == 0 " << " parent? "
    //            << (parent_collection != nullptr) << " type? "
    //            << (parent_collection
    //                    ? static_cast<int>(parent_collection->type())
    //                    : -1);
    level_ = 0;
  } else {
    auto* parent_tree_node =
        static_cast<TreeTabNodeTabCollection*>(parent_collection);
    level_ = parent_tree_node->node().level_ + 1;
    // LOG(ERROR) << "TreeTabNode level updated: " << level_;
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
