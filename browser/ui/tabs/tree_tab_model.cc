// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/tabs/tree_tab_model.h"

#include <cstddef>
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
  if (!node || node->collapsed() == collapsed) {
    return;
  }
  node->set_collapsed(collapsed);

  if (!node->height()) {
    return;
  }

  if (collapsed) {
    // When collapsed, mark descendants as belonging to the collapsed node.
    std::vector<tree_tab::TreeTabNodeId> descendant_ids;
    node->CollectUncollapseDescendantIds(descendant_ids);
    for (const auto& desc_id : descendant_ids) {
      closest_collapsed_ancestor_.insert_or_assign(desc_id, id);
      descendant_ids_by_collapsed_ancestor_[id].insert(desc_id);
    }
  } else {
    // When uncollapsed, descendants pointing to this node as their closest
    // collapsed ancestor are invalidated, so we need to recompute the closest
    // collapsed ancestor for each descendant.
    auto* descendants =
        base::FindOrNull(descendant_ids_by_collapsed_ancestor_, id);
    if (!descendants) {
      return;
    }

    std::set<tree_tab::TreeTabNodeId> to_recompute = std::move(*descendants);

    // This node is no longer a collapsed ancestor, so remove its entry from the
    // cache.
    descendant_ids_by_collapsed_ancestor_.erase(id);

    // Recompute the closest collapsed ancestor for each descendant.
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
        descendant_ids_by_collapsed_ancestor_[*new_ancestor].insert(node_id);
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
    descendant_ids_by_collapsed_ancestor_[*closest].insert(node.id());
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
    if (!node_to_update) {
      // Node was already removed (e.g. during teardown); RemoveTreeTabNode
      // clears the cache. Defensively erase forward map in case it wasn't.
      closest_collapsed_ancestor_.erase(node_id);
      continue;
    }

    // Note that we copy the value of old_ancestor to another value as it could
    // be invalidated by the next operation: erasing.
    const auto* found = base::FindOrNull(closest_collapsed_ancestor_, node_id);
    std::optional<tree_tab::TreeTabNodeId> old_ancestor =
        found ? std::optional(*found) : std::nullopt;

    std::optional<tree_tab::TreeTabNodeId> closest =
        node_to_update->GetClosestCollapsedAncestorId();
    if (closest) {
      closest_collapsed_ancestor_.insert_or_assign(node_id, *closest);
      descendant_ids_by_collapsed_ancestor_[*closest].insert(node_id);
    } else {
      closest_collapsed_ancestor_.erase(node_id);
    }

    if (old_ancestor.has_value() && old_ancestor != closest) {
      auto* descendants = base::FindOrNull(
          descendant_ids_by_collapsed_ancestor_, *old_ancestor);
      if (descendants) {
        descendants->erase(node_id);
        if (descendants->empty()) {
          descendant_ids_by_collapsed_ancestor_.erase(*old_ancestor);
        }
      }
    }
  }
}

void TreeTabModel::RemoveTreeTabNode(const tree_tab::TreeTabNodeId& id) {
  if (!GetNode(id)) {
    return;
  }

  // Update closest collapsed ancestor cache before removing.
  // 1. Remove this node from its (former) closest collapsed ancestor's set.
  const auto* found = base::FindOrNull(closest_collapsed_ancestor_, id);
  std::optional<tree_tab::TreeTabNodeId> old_ancestor =
      found ? std::optional(*found) : std::nullopt;
  closest_collapsed_ancestor_.erase(id);
  if (old_ancestor.has_value()) {
    auto* descendants =
        base::FindOrNull(descendant_ids_by_collapsed_ancestor_, *old_ancestor);
    if (descendants) {
      descendants->erase(id);
      if (descendants->empty()) {
        descendant_ids_by_collapsed_ancestor_.erase(*old_ancestor);
      }
    }
  }

  // 2. Recompute nodes that had this node as their closest collapsed ancestor.
  if (auto* descendants =
          base::FindOrNull(descendant_ids_by_collapsed_ancestor_, id)) {
    for (const auto& descendant_id : *descendants) {
      const tabs::TreeTabNode* node_to_update = GetNode(descendant_id);
      if (!node_to_update) {
        // Descendant no longer in model (already removed); clean up its cache
        // entry
        closest_collapsed_ancestor_.erase(descendant_id);
        continue;
      }

      if (std::optional<tree_tab::TreeTabNodeId> new_ancestor =
              node_to_update->GetClosestCollapsedAncestorId()) {
        closest_collapsed_ancestor_.insert_or_assign(descendant_id,
                                                     *new_ancestor);
        descendant_ids_by_collapsed_ancestor_[*new_ancestor].insert(
            descendant_id);
      } else {
        closest_collapsed_ancestor_.erase(descendant_id);
      }
    }

    // Drop removed node's entry from the cache. Note that we should do this
    // after recomputing the closest collapsed ancestor for the descendants,
    // as erasing the entry will invalidate the |descendants|.
    descendant_ids_by_collapsed_ancestor_.erase(id);
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
