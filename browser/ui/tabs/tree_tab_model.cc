// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/tabs/tree_tab_model.h"

#include <optional>
#include <vector>

#include "base/containers/map_util.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/tabs/public/tree_tab_node_id.h"

TreeTabModel::TreeTabModel() = default;
TreeTabModel::~TreeTabModel() = default;

const tabs::TreeTabNode* TreeTabModel::GetNode(
    const tree_tab::TreeTabNodeId& id) const {
  const auto* entry = base::FindOrNull(tree_tab_nodes_, id);
  return entry ? entry->get() : nullptr;
}

tabs::TreeTabNode* TreeTabModel::GetNode(const tree_tab::TreeTabNodeId& id) {
  auto* entry = base::FindOrNull(tree_tab_nodes_, id);
  return entry ? entry->get() : nullptr;
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
  tabs::TreeTabNode* node = GetNode(id);
  if (!node) {
    return;
  }
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
      const tabs::TreeTabNode* node_to_recompute = GetNode(node_id);
      if (!node_to_recompute) {
        closest_collapsed_ancestor_.erase(node_id);
        continue;
      }
      std::optional<tree_tab::TreeTabNodeId> new_ancestor =
          node_to_recompute->GetClosestCollapsedAncestorId();
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

    if (const tabs::TreeTabNode* node = model->GetNode(id)) {
      model->add_tree_tab_node_callback_list_.Notify(*node);
    }
  };

  // Defer notification to make sure the tab creation operation is completed
  // before wrapping it up with TreeTabNode created here.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(notification), GetWeakPtr(), node.id()));
}

void TreeTabModel::OnTreeTabNodeMoved(const tree_tab::TreeTabNodeId& id) {
  tabs::TreeTabNode* moving_node = GetNode(id);
  if (!moving_node) {
    // As this callback is called whenever reparenting a node, it is possible
    // that the node is still in the middle of creation, so AddTreeTabNode
    // hasn't been called yet.
    return;
  }

  // The moving node and its descendants should recalculate their closest
  // collapsed ancestor.
  std::vector<tree_tab::TreeTabNodeId> to_update = {id};
  moving_node->CollectDescendantIds(to_update);

  for (const auto& node_id : to_update) {
    const tabs::TreeTabNode* node_to_update = GetNode(node_id);
    CHECK(node_to_update);

    if (std::optional<tree_tab::TreeTabNodeId> closest =
            node_to_update->GetClosestCollapsedAncestorId()) {
      closest_collapsed_ancestor_.insert_or_assign(node_id, *closest);
    } else {
      closest_collapsed_ancestor_.erase(node_id);
    }
  }
}

void TreeTabModel::RemoveTreeTabNode(const tree_tab::TreeTabNodeId& id) {
  if (!GetNode(id)) {
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

  tree_tab_nodes_.erase(id);
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
