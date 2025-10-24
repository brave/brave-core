// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/tabs/tree_tab_model.h"

TreeTabModel::TreeTabModel() = default;
TreeTabModel::~TreeTabModel() = default;

const tabs::TreeTabNode* TreeTabModel::GetNode(
    const tree_tab::TreeTabNodeId& id) const {
  return tree_tab_nodes_.contains(id) ? nullptr : tree_tab_nodes_.at(id);
}

int TreeTabModel::GetTreeHeight(const tree_tab::TreeTabNodeId& id) const {
  const auto* node = GetNode(id);
  if (!node) {
    return 0;
  }

  return node->GetTreeHeight();
}

void TreeTabModel::AddTreeTabNode(const tabs::TreeTabNode& node) {
  if (tree_tab_nodes_.contains(node.id())) {
    return;
  }

  tree_tab_nodes_[node.id()] = &node;
}

void TreeTabModel::RemoveTreeTabNode(const tree_tab::TreeTabNodeId& id) {
  if (!tree_tab_nodes_.contains(id)) {
    return;
  }

  tree_tab_nodes_.erase(id);
}

base::WeakPtr<TreeTabModel> TreeTabModel::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}
