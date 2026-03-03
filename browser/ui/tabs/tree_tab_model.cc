// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/tabs/tree_tab_model.h"

#include <optional>
#include <vector>

#include "base/task/sequenced_task_runner.h"
#include "brave/components/tabs/public/tree_tab_node_id.h"

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

void TreeTabModel::SetCollapsed(const tree_tab::TreeTabNodeId& id,
                                bool collapsed) {
  auto iter = tree_tab_nodes_.find(id);
  if (iter == tree_tab_nodes_.end()) {
    return;
  }
  tabs::TreeTabNode* node = iter->second;
  node->set_collapsed(collapsed);

  if (collapsed) {
    std::vector<tree_tab::TreeTabNodeId> descendant_ids;
    node->CollectDescendantIds(descendant_ids);
    for (const auto& desc_id : descendant_ids) {
      closest_collapsed_ancestor_.insert_or_assign(desc_id, id);
    }
  } else {
    std::vector<tree_tab::TreeTabNodeId> to_recompute;
    for (const auto& [node_id, ancestor_id] : closest_collapsed_ancestor_) {
      if (ancestor_id == id) {
        to_recompute.push_back(node_id);
      }
    }
    for (const auto& node_id : to_recompute) {
      auto node_iter = tree_tab_nodes_.find(node_id);
      if (node_iter == tree_tab_nodes_.end()) {
        closest_collapsed_ancestor_.erase(node_id);
        continue;
      }
      std::optional<tree_tab::TreeTabNodeId> new_ancestor =
          node_iter->second->GetClosestCollapsedAncestorId();
      if (new_ancestor.has_value()) {
        closest_collapsed_ancestor_.insert_or_assign(node_id, *new_ancestor);
      } else {
        closest_collapsed_ancestor_.erase(node_id);
      }
    }
  }
}

bool TreeTabModel::DoesBelongToCollapsedNode(
    const tree_tab::TreeTabNodeId& id) const {
  return closest_collapsed_ancestor_.count(id) != 0;
}

void TreeTabModel::AddTreeTabNode(tabs::TreeTabNode& node) {
  if (tree_tab_nodes_.contains(node.id())) {
    return;
  }

  tree_tab_nodes_[node.id()] = &node;

  if (std::optional<tree_tab::TreeTabNodeId> closest =
          node.GetClosestCollapsedAncestorId()) {
    closest_collapsed_ancestor_.insert_or_assign(node.id(), *closest);
  }

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

void TreeTabModel::OnTreeTabNodeMoved(const tree_tab::TreeTabNodeId& id) {
  auto iter = tree_tab_nodes_.find(id);
  if (iter == tree_tab_nodes_.end()) {
    // As this callback is called whenever reparenting a node, it is possible
    // that the node is still in the middle of creation, so AddTreeTabNode
    // hasn't been called yet.
    return;
  }

  tabs::TreeTabNode* node = iter->second;

  // The moving node and its descendants should recalculate their closest
  // collapsed ancestor.
  std::vector<tree_tab::TreeTabNodeId> to_update = {id};
  node->CollectDescendantIds(to_update);

  for (const auto& node_id : to_update) {
    auto node_it = tree_tab_nodes_.find(node_id);
    CHECK(node_it != tree_tab_nodes_.end());

    if (std::optional<tree_tab::TreeTabNodeId> closest =
            node_it->second->GetClosestCollapsedAncestorId()) {
      closest_collapsed_ancestor_.insert_or_assign(node_id, *closest);
    } else {
      closest_collapsed_ancestor_.erase(node_id);
    }
  }
}

void TreeTabModel::RemoveTreeTabNode(const tree_tab::TreeTabNodeId& id) {
  auto iter = tree_tab_nodes_.find(id);
  if (iter == tree_tab_nodes_.end()) {
    return;
  }

  // Update closest collapsed ancestor cache before removing
  // 1. drop this node's entry and
  closest_collapsed_ancestor_.erase(id);

  // 2. recompute nodes that had this node as their closest collapsed ancestor.
  std::vector<tree_tab::TreeTabNodeId> to_recompute;
  for (const auto& [node_id, ancestor_id] : closest_collapsed_ancestor_) {
    if (ancestor_id == id) {
      to_recompute.push_back(node_id);
    }
  }
  for (const auto& node_id : to_recompute) {
    const tabs::TreeTabNode* node = GetNode(node_id);
    CHECK(node);

    if (std::optional<tree_tab::TreeTabNodeId> new_ancestor =
            node->GetClosestCollapsedAncestorId()) {
      closest_collapsed_ancestor_.insert_or_assign(node_id, *new_ancestor);
    } else {
      closest_collapsed_ancestor_.erase(node_id);
    }
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
