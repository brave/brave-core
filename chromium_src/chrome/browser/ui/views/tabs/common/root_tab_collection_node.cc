// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/tabs/common/root_tab_collection_node.h"

#include <variant>
#include <vector>

#include "base/functional/bind.h"
#include "base/stl_util.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/views/tabs/tree_tab_view_utils.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/tabs/common/tab_collection_node.h"
#include "chrome/browser/ui/views/tabs/common/tab_group_view.h"
#include "chrome/browser/ui/views/tabs/common/tab_strip_collection_controller.h"
#include "chrome/browser/ui/views/tabs/common/tab_view.h"
#include "components/tab_groups/tab_group_id.h"
#include "components/tabs/public/split_tab_data.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_collection_types.h"
#include "components/tabs/public/tab_group.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_controller.h"

#define OnChildrenAdded OnChildrenAdded_ChromiumImpl
#define OnChildrenRemoved OnChildrenRemoved_ChromiumImpl
#define OnChildMoved OnChildMoved_ChromiumImpl
#define OnTabStripModelChanged OnTabStripModelChanged_ChromiumImpl

#include <chrome/browser/ui/views/tabs/common/root_tab_collection_node.cc>

#undef OnTabStripModelChanged
#undef OnChildMoved
#undef OnChildrenRemoved
#undef OnChildrenAdded

namespace {

tabs::TabCollectionNodeHandle GetHandleForNodeData(
    const tabs::ConstChildPtr& node_data) {
  if (const auto* collection =
          std::get_if<tabs::ConstDanglingUntriagedTabCollection>(&node_data)) {
    return (*collection)->GetHandle();
  }
  return std::get<tabs::ConstDanglingUntriagedTabInterface>(node_data)
      ->GetHandle();
}

const tabs::TabCollection* GetUnpinnedCollection(
    const TabStripModel* tab_strip_model) {
  for (const auto& child : tab_strip_model->Root()->GetChildren()) {
    if (const auto* collection =
            std::get_if<std::unique_ptr<tabs::TabCollection>>(&child);
        collection &&
        (*collection)->type() == tabs::TabCollection::Type::UNPINNED) {
      return collection->get();
    }
  }
  return nullptr;
}

}  // namespace

void RootTabCollectionNode::OnChildrenAdded(
    const tabs::TabCollection::Position& position,
    const tabs::TabCollectionNodes& handles,
    bool insert_from_detached) {
  if (ShouldSyncWithTreeTabs({position.parent_handle})) {
    SyncWithTreeTabs();
    return;
  }
  OnChildrenAdded_ChromiumImpl(position, handles, insert_from_detached);
}

void RootTabCollectionNode::OnChildrenRemoved(
    const tabs::TabCollection::Position& position,
    const tabs::TabCollectionNodes& handles) {
  if (ShouldSyncWithTreeTabs({position.parent_handle})) {
    SyncWithTreeTabs();
    return;
  }
  OnChildrenRemoved_ChromiumImpl(position, handles);
}

void RootTabCollectionNode::OnChildMoved(
    const tabs::TabCollection::Position& to_position,
    const NodeData& node_data) {
  if (ShouldSyncWithTreeTabs(
          {to_position.parent_handle, node_data.position.parent_handle})) {
    SyncWithTreeTabs();
    return;
  }
  OnChildMoved_ChromiumImpl(to_position, node_data);
}

void RootTabCollectionNode::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  // The tree tab delegate could have restructured the hierarchy without
  // sending any collection notification.
  if (ShouldSyncWithTreeTabs({})) {
    SyncWithTreeTabs();
  }
  OnTabStripModelChanged_ChromiumImpl(tab_strip_model, change, selection);

  // Make sure the active tab is visible. Other selected tabs could be in a
  // collapsed tree, e.g. descendants selected together with their collapsed
  // parent for dragging.
  if (selection.active_tab_changed() && !tab_strip_model->closing_all()) {
    ExpandCollapsedTreeTabAncestors(selection.new_tab);
  }
}

void RootTabCollectionNode::OnTreeTabChanged(const TreeTabChange& change) {
  // Tree tab changes are notified in the middle of collection updates, so the
  // hierarchy may not be in a consistent state yet. Sync later.
  ScheduleSyncWithTreeTabs();

  if (change.type == TreeTabChange::Type::kNodeCollapsedStateChanged ||
      change.type == TreeTabChange::Type::kNodeChildrenChanged ||
      change.type == TreeTabChange::Type::kNodeReparented) {
    if (auto* unpinned = GetChildNodeOfType(Type::UNPINNED);
        unpinned && unpinned->view()) {
      unpinned->view()->InvalidateLayout();
    }
  }
}

bool RootTabCollectionNode::ShouldSyncWithTreeTabs(
    const std::vector<tabs::TabCollection::Handle>& parent_handles) {
  if (tree_tabs_sync_pending_) {
    return true;
  }

  if (const auto* unpinned = GetUnpinnedCollection(tab_strip_model_);
      unpinned && tree_tab_view_utils::HasTreeTabNodeChild(*unpinned)) {
    return true;
  }

  // The notification refers to a collection that isn't mirrored in the view
  // model, e.g. a tree tab node collection that was removed right after.
  for (const auto& handle : parent_handles) {
    if (!GetNodeForHandle(handle)) {
      return true;
    }
  }
  return false;
}

void RootTabCollectionNode::ScheduleSyncWithTreeTabs() {
  if (tree_tabs_sync_pending_) {
    return;
  }
  tree_tabs_sync_pending_ = true;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&RootTabCollectionNode::SyncWithTreeTabs,
                                weak_ptr_factory_.GetWeakPtr()));
}

void RootTabCollectionNode::SyncWithTreeTabs() {
  tree_tabs_sync_pending_ = false;
  if (!view()) {
    // Not initialized yet or already reset.
    return;
  }

  TreeTabsSyncResult result;
  // Add or move nodes first so that existing views are reused, then remove
  // what's left over.
  SyncChildrenWithTreeTabs(this, result);
  RemoveStaleChildren(this, result);

  if (!result.added.empty()) {
    on_children_added_callback_list_.Notify(result.added);
  }
  if (result.removed) {
    on_children_removed_callback_list_.Notify();
  }
  for (const auto& handle : result.moved) {
    if (TabCollectionNode* moved_node = GetNodeForHandle(handle)) {
      on_child_moved_callback_list_.Notify(moved_node);
    }
  }

  // Tree tab node states could have been changed, e.g. collapsed or got new
  // descendants.
  if (TabCollectionNode* unpinned = GetChildNodeOfType(Type::UNPINNED)) {
    for (const auto& child : unpinned->children()) {
      if (auto* tab_view = views::AsViewClass<TabView>(child->view())) {
        tab_view->UpdateTreeToggleButton();
      }
    }
  }

  // e.g. A new active tab could be added under a collapsed tree tab node.
  if (!tab_strip_model_->closing_all()) {
    ExpandCollapsedTreeTabAncestors(tab_strip_model_->GetActiveTab());
  }
}

void RootTabCollectionNode::SyncChildrenWithTreeTabs(
    TabCollectionNode* node,
    TreeTabsSyncResult& result) {
  const tabs::ConstChildPtr node_data = node->GetNodeData();
  const auto* collection =
      std::get_if<tabs::ConstDanglingUntriagedTabCollection>(&node_data);
  if (!collection) {
    return;
  }

  const tabs::ConstChildrenPtrs desired =
      tree_tab_view_utils::GetChildrenForView(**collection);
  for (size_t i = 0; i < desired.size(); ++i) {
    const tabs::TabCollectionNodeHandle handle =
        GetHandleForNodeData(desired[i]);
    if (i >= node->children().size() ||
        node->children()[i]->GetHandle() != handle) {
      TabCollectionNode* current_parent = GetParentNodeForHandle(handle);
      if (current_parent == node) {
        node->MoveChild(GetPassKey(), handle, i);
        result.moved.push_back(handle);
      } else if (current_parent && IsInPinnedContainer(current_parent) ==
                                       IsInPinnedContainer(node)) {
        TabCollectionNode::MoveChild(GetPassKey(), handle, i, current_parent,
                                     node);
        result.moved.push_back(handle);
      } else {
        // Pin state changes are treated as remove and add, same as
        // OnChildMoved().
        if (current_parent) {
          on_child_will_be_removed_callback_list_.Notify(
              current_parent->GetNodeForHandle(handle));
          current_parent->RemoveChild(GetPassKey(), handle,
                                      /*perform_deinitialization=*/false);
          result.removed = true;
        }
        // Children are synced below so that existing child views are reused.
        node->AddNewChild(GetPassKey(), desired[i], i,
                          /*perform_initialization=*/false);
        result.added.push_back(handle);
      }
    }

    CHECK(node->children()[i]->GetHandle() == handle);
    SyncChildrenWithTreeTabs(node->children()[i].get(), result);
  }
}

void RootTabCollectionNode::RemoveStaleChildren(TabCollectionNode* node,
                                                TreeTabsSyncResult& result) {
  const tabs::ConstChildPtr node_data = node->GetNodeData();
  const auto* collection =
      std::get_if<tabs::ConstDanglingUntriagedTabCollection>(&node_data);
  if (!collection) {
    return;
  }

  // After SyncChildrenWithTreeTabs(), the leading children match the
  // collection and the trailing ones are stale. Stale nodes can refer to
  // destroyed tabs or collections, so only use their handles.
  const size_t desired_count =
      tree_tab_view_utils::GetChildrenForView(**collection).size();
  while (node->children().size() > desired_count) {
    TabCollectionNode* stale = node->children().back().get();
    on_child_will_be_removed_callback_list_.Notify(stale);
    node->RemoveChild(GetPassKey(), stale->GetHandle(),
                      /*perform_deinitialization=*/false);
    result.removed = true;
  }

  for (const auto& child : node->children()) {
    RemoveStaleChildren(child.get(), result);
  }
}

bool RootTabCollectionNode::IsInPinnedContainer(const TabCollectionNode* node) {
  TabCollectionNode* pinned = GetChildNodeOfType(Type::PINNED);
  return pinned && pinned->GetNodeForHandle(node->GetHandle());
}

void RootTabCollectionNode::ExpandCollapsedTreeTabAncestors(
    const tabs::TabInterface* tab) {
  if (!tab) {
    return;
  }

  // |tab| could be in a split or a group wrapped by a tree tab node.
  const tabs::TreeTabNode* node =
      tree_tab_view_utils::GetTreeTabNode(tabs::ConstChildPtr(tab));
  for (const tabs::TabCollection* parent = tab->GetParentCollection();
       parent && !node; parent = parent->GetParentCollection()) {
    node = tree_tab_view_utils::GetTreeTabNode(tabs::ConstChildPtr(parent));
  }
  if (!node) {
    return;
  }

  // Tree tab nodes only exist with BraveTabStripModel.
  auto* model = static_cast<BraveTabStripModel*>(tab_strip_model_.get());
  while (auto collapsed_ancestor = node->GetClosestCollapsedAncestorId()) {
    model->SetTreeTabNodeCollapsed(*collapsed_ancestor, false);
    if (node->GetClosestCollapsedAncestorId() == collapsed_ancestor) {
      // Failed to expand.
      return;
    }
  }
}
