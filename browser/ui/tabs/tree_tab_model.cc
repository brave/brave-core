// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/tabs/tree_tab_model.h"

#include "base/logging.h"
#include "base/task/sequenced_task_runner.h"

TreeTabModel::TreeTabModel() = default;
TreeTabModel::~TreeTabModel() = default;

const tabs::TreeTabNode* TreeTabModel::GetNode(
    const tree_tab::TreeTabNodeId& id) const {
  auto iter = tree_tab_nodes_.find(id);
  return iter == tree_tab_nodes_.end() ? nullptr : iter->second;
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

  auto notification = [](base::WeakPtr<TreeTabModel> model,
                         const tree_tab::TreeTabNodeId& id) {
    if (!model) {
      return;
    }

    auto iter = model->tree_tab_nodes_.find(id);
    if (iter == model->tree_tab_nodes_.end()) {
      return;
    }
    model->add_tree_tab_node_callback_list_.Notify(*iter->second);
  };

  // Defer notification to make sure the tab creation operation is completed
  // before wrapping it up with TreeTabNode created here.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(notification), GetWeakPtr(), node.id()));
}

void TreeTabModel::RemoveTreeTabNode(const tree_tab::TreeTabNodeId& id) {
  auto iter = tree_tab_nodes_.find(id);
  if (iter == tree_tab_nodes_.end()) {
    return;
  }

  will_remove_tree_tab_node_callback_list_.Notify(id);

  tree_tab_nodes_.erase(iter);
}

base::WeakPtr<TreeTabModel> TreeTabModel::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

base::CallbackListSubscription TreeTabModel::RegisterAddTreeTabNodeCallback(
    base::RepeatingCallback<void(const tabs::TreeTabNode&)> callback) {
  return add_tree_tab_node_callback_list_.Add(std::move(callback));
}

base::CallbackListSubscription
TreeTabModel::RegisterWillRemoveTreeTabNodeCallback(
    base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> callback) {
  return will_remove_tree_tab_node_callback_list_.Add(std::move(callback));
}
