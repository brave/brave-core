// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <variant>

#include "absl/functional/overload.h"
#include "base/check_op.h"
#include "base/containers/adapters.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/types/to_address.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "components/tabs/public/split_tab_collection.h"
#include "components/tabs/public/tab_group_tab_collection.h"
#include "components/tabs/public/tab_interface.h"

namespace tabs {

// static
void TreeTabNodeTabCollection::BuildTreeTabs(
    TabCollection& root,
    base::RepeatingCallback<void(TreeTabNode& node)> on_create,
    base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> on_remove,
    base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> on_move) {
  auto tabs = root.GetTabsRecursive();
  base::flat_set<TabCollection*> processed_splits;
  base::flat_set<TabCollection*> processed_groups;

  // Process from back to front so replacing a tab/split/group at |index| does
  // not change indices of items we have not yet processed (order preserved).
  while (!tabs.empty()) {
    auto* tab_interface = tabs.back();
    tabs.pop_back();

    auto* parent_collection =
        tab_interface->GetParentCollection(GetPassKeyStatic());
    CHECK(parent_collection) << "Tab should always have a parent collection.";
    CHECK_NE(parent_collection->type(), TabCollection::Type::PINNED)
        << "Pinned tabs should not be wrapped in TreeTabNode.";

    // SplitTabCollection does not support child collections. Wrap the entire
    // split in one tree node and add it to the grandparent.
    if (parent_collection->type() == TabCollection::Type::SPLIT) {
      if (processed_splits.contains(parent_collection)) {
        continue;
      }
      processed_splits.insert(parent_collection);

      TabCollection* grandparent = parent_collection->GetParentCollection();
      CHECK(grandparent);
      size_t index =
          grandparent->GetIndexOfCollection(parent_collection).value();
      std::unique_ptr<TabCollection> removed =
          grandparent->MaybeRemoveCollection(parent_collection);
      auto tree_node = std::make_unique<TreeTabNodeTabCollection>(
          tree_tab::TreeTabNodeId::GenerateNew(),
          base::WrapUnique<SplitTabCollection>(
              static_cast<SplitTabCollection*>(removed.release())),
          on_remove, on_move);
      TreeTabNode& node_ref = tree_node->node();
      grandparent->AddCollection(std::move(tree_node), index);
      on_create.Run(node_ref);
      continue;
    }

    // Wrap the entire group in one tree node and add it to the grandparent.
    // Group tabs stay as direct children of the group (no per-tab tree nodes).
    if (parent_collection->type() == TabCollection::Type::GROUP) {
      if (processed_groups.contains(parent_collection)) {
        continue;
      }
      processed_groups.insert(parent_collection);

      TabCollection* grandparent = parent_collection->GetParentCollection();
      CHECK(grandparent);
      size_t index =
          grandparent->GetIndexOfCollection(parent_collection).value();
      std::unique_ptr<TabCollection> removed =
          grandparent->MaybeRemoveCollection(parent_collection);
      auto tree_node = std::make_unique<TreeTabNodeTabCollection>(
          tree_tab::TreeTabNodeId::GenerateNew(),
          base::WrapUnique<TabGroupTabCollection>(
              static_cast<TabGroupTabCollection*>(removed.release())),
          on_remove, on_move);
      TreeTabNode& node_ref = tree_node->node();
      grandparent->AddCollection(std::move(tree_node), index);
      on_create.Run(node_ref);
      continue;
    }

    int index = *parent_collection->GetIndexOfTab(tab_interface);
    auto owned_tab_interface = parent_collection->MaybeRemoveTab(tab_interface);
    CHECK_EQ(tab_interface, owned_tab_interface.get());

    auto tree_node = std::make_unique<TreeTabNodeTabCollection>(
        tree_tab::TreeTabNodeId::GenerateNew(), std::move(owned_tab_interface),
        on_remove, on_move);
    TreeTabNode& node_ref = tree_node->node();
    parent_collection->AddCollection(std::move(tree_node), index);
    on_create.Run(node_ref);
  }
}

void TreeTabNodeTabCollection::FlattenTreeTabs(TabCollection& root) {
  // Get all TreeTabNodes recursively from the root.
  std::vector<TreeTabNodeTabCollection*> all_tree_nodes;
  CollectTreeNodesRecursively(root, all_tree_nodes);

  // Process tree nodes in reverse order so we handle children before parents.
  for (auto* tree_node : base::Reversed(all_tree_nodes)) {
    // Move all direct children (tabs and collections e.g. SplitTabCollection)
    // from this tree node back to the tree node's parent, preserving order.
    std::vector<std::variant<tabs::TabInterface*, TabCollection*>> children =
        tree_node->GetTreeNodeChildren();
    auto* parent_collection = tree_node->GetParentCollection();
    CHECK(parent_collection)
        << "Tree node should always have a parent collection.";
    size_t index = parent_collection->GetIndexOfCollection(tree_node).value();

    // Before detaching children, we need to notify that the tree node is about
    // to be destroyed.
    if (!tree_node->on_remove_.is_null()) {
      tree_node->on_remove_.Run(tree_node->node().id());
    }

    // Remove all children in order, then insert back at the parent in the same
    // order (at index, index+1, ...) so tab/collection order is preserved.
    using RemovedChild = std::variant<std::unique_ptr<TabInterface>,
                                      std::unique_ptr<TabCollection>>;
    std::vector<RemovedChild> removed;
    removed.reserve(children.size());
    for (const auto& child : children) {
      std::visit(
          absl::Overload{[&](tabs::TabInterface* tab) {
                           removed.push_back(tree_node->MaybeRemoveTab(tab));
                         },
                         [&](tabs::TabCollection* collection) {
                           removed.push_back(
                               tree_node->MaybeRemoveCollection(collection));
                         }},
          child);
    }

    for (auto& item : removed) {
      std::visit(
          absl::Overload{[&](std::unique_ptr<TabInterface>& tab) {
                           parent_collection->AddTab(std::move(tab), index++);
                         },
                         [&](std::unique_ptr<TabCollection>& collection) {
                           parent_collection->AddCollection(
                               std::move(collection), index++);
                         }},
          item);
    }

    CHECK(tree_node->GetTreeNodeChildren().empty())
        << "Tree node should not have any children at this point.";
    std::ignore = parent_collection->MaybeRemoveCollection(tree_node);
  }
}

TreeTabNodeTabCollection::TreeTabNodeTabCollection(
    const tree_tab::TreeTabNodeId& tree_tab_node_id,
    std::unique_ptr<tabs::TabInterface> current_tab,
    base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> on_remove,
    base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> on_move)
    : TabCollection(TabCollection::Type::TREE_NODE,
                    /*supported_child_collections=*/
                    {TabCollection::Type::SPLIT, TabCollection::Type::GROUP,
                     TabCollection::Type::TREE_NODE},
                    /*supports_tabs=*/true),
      current_value_type_(CurrentValueType::kTab),
      on_remove_(std::move(on_remove)),
      on_move_(std::move(on_move)),
      node_(std::make_unique<TreeTabNode>(*this, tree_tab_node_id)) {
  CHECK(!tree_tab_node_id.is_empty());
  if (current_tab) {
    current_value_ = current_tab->GetWeakPtr();
    CHECK(std::get<base::WeakPtr<tabs::TabInterface>>(*current_value_));
    AddTab(std::move(current_tab), 0);
  } else {
    // Used by GetEmptyTreeTabNode()
    current_value_ = base::WeakPtr<tabs::TabInterface>();
  }
}

TreeTabNodeTabCollection::TreeTabNodeTabCollection(
    const tree_tab::TreeTabNodeId& tree_tab_node_id,
    std::unique_ptr<tabs::SplitTabCollection> current_collection,
    base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> on_remove,
    base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> on_move)
    : TabCollection(TabCollection::Type::TREE_NODE,
                    /*supported_child_collections=*/
                    {TabCollection::Type::SPLIT, TabCollection::Type::GROUP,
                     TabCollection::Type::TREE_NODE},
                    /*supports_tabs=*/true),
      current_value_type_(CurrentValueType::kSplit),
      on_remove_(std::move(on_remove)),
      on_move_(std::move(on_move)),
      node_(std::make_unique<TreeTabNode>(*this, tree_tab_node_id)) {
  CHECK(!tree_tab_node_id.is_empty());
  CHECK(current_collection);
  auto* split_ptr = static_cast<SplitTabCollection*>(current_collection.get());
  CHECK(split_ptr);
  current_value_ = split_ptr;
  AddCollection(std::move(current_collection), 0);
}

TreeTabNodeTabCollection::TreeTabNodeTabCollection(
    const tree_tab::TreeTabNodeId& tree_tab_node_id,
    std::unique_ptr<tabs::TabGroupTabCollection> current_collection,
    base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> on_remove,
    base::RepeatingCallback<void(const tree_tab::TreeTabNodeId&)> on_move)
    : TabCollection(TabCollection::Type::TREE_NODE,
                    /*supported_child_collections=*/
                    {TabCollection::Type::SPLIT, TabCollection::Type::GROUP,
                     TabCollection::Type::TREE_NODE},
                    /*supports_tabs=*/true),
      current_value_type_(CurrentValueType::kGroup),
      on_remove_(std::move(on_remove)),
      on_move_(std::move(on_move)),
      node_(std::make_unique<TreeTabNode>(*this, tree_tab_node_id)) {
  CHECK(!tree_tab_node_id.is_empty());
  CHECK(current_collection);
  current_value_ = current_collection.get();
  AddCollection(std::move(current_collection), 0);
}

TreeTabNodeTabCollection::~TreeTabNodeTabCollection() {
  if (on_remove_) {
    on_remove_.Run(node_->id());
  }
}

tabs::TabInterface* TreeTabNodeTabCollection::GetCurrentTab() const {
  CHECK_EQ(current_value_type_, CurrentValueType::kTab);
  return std::get<base::WeakPtr<tabs::TabInterface>>(*current_value_).get();
}

tabs::TabCollection* TreeTabNodeTabCollection::GetCurrentCollection() const {
  CHECK_NE(current_value_type_, CurrentValueType::kTab);
  if (std::holds_alternative<raw_ptr<tabs::SplitTabCollection>>(
          *current_value_)) {
    return static_cast<tabs::TabCollection*>(
        std::get<raw_ptr<tabs::SplitTabCollection>>(*current_value_));
  }

  CHECK(std::holds_alternative<raw_ptr<tabs::TabGroupTabCollection>>(
      *current_value_));
  return static_cast<tabs::TabCollection*>(
      std::get<raw_ptr<tabs::TabGroupTabCollection>>(*current_value_));
}

TreeTabNodeTabCollection* TreeTabNodeTabCollection::GetTopLevelAncestor() {
  auto* parent = GetParentCollection();
  if (!parent || parent->type() != TabCollection::Type::TREE_NODE) {
    return this;
  }
  return static_cast<TreeTabNodeTabCollection*>(parent)->GetTopLevelAncestor();
}

const TreeTabNodeTabCollection* TreeTabNodeTabCollection::GetTopLevelAncestor()
    const {
  return const_cast<TreeTabNodeTabCollection*>(this)->GetTopLevelAncestor();
}

// static
void TreeTabNodeTabCollection::CollectTreeNodesRecursively(
    tabs::TabCollection& parent,
    std::vector<TreeTabNodeTabCollection*>& nodes) {
  for (auto& child : GetChildrenStatic(parent)) {
    if (std::holds_alternative<std::unique_ptr<tabs::TabInterface>>(child)) {
      continue;  // Skip tabs, we only want collections.
    }

    auto& collection = std::get<std::unique_ptr<tabs::TabCollection>>(child);
    if (collection->type() == TabCollection::Type::TREE_NODE) {
      auto* tree_node =
          static_cast<TreeTabNodeTabCollection*>(collection.get());
      nodes.push_back(tree_node);
    }

    // Recursively collect from all collections (including groups).
    CollectTreeNodesRecursively(*collection, nodes);
  }
}

std::vector<std::variant<tabs::TabInterface*, tabs::TabCollection*>>
TreeTabNodeTabCollection::GetTreeNodeChildren() {
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

std::unique_ptr<TabCollection> TreeTabNodeTabCollection::MaybeRemoveCollection(
    TabCollection* collection) {
  const bool clear_current_value =
      (current_value_type_ == CurrentValueType::kGroup ||
       current_value_type_ == CurrentValueType::kSplit) &&
      GetCurrentCollection() == collection;

  std::unique_ptr<TabCollection> removed =
      TabCollection::MaybeRemoveCollection(collection);

  if (clear_current_value) {
    // The group/split object may later be destroyed (e.g. CloseDetachedTabGroup
    // after the last tab is removed). Drop our raw_ptr so it cannot dangle.
    if (current_value_type_ == CurrentValueType::kGroup) {
      current_value_ = static_cast<tabs::TabGroupTabCollection*>(nullptr);
    } else if (current_value_type_ == CurrentValueType::kSplit) {
      current_value_ = static_cast<tabs::SplitTabCollection*>(nullptr);
    }
  }

  return removed;
}

void TreeTabNodeTabCollection::OnReparented(TabCollection* new_parent) {
  auto* old_parent = GetParentCollection();

  TabCollection::OnReparented(new_parent);

  node_->CalculateLevelAndHeightRecursively({});

  CHECK_NE(old_parent, new_parent);

  if (new_parent) {
    if (new_parent->type() == TabCollection::Type::TREE_NODE) {
      static_cast<TreeTabNodeTabCollection*>(new_parent)
          ->node_->OnChildHeightChanged({});
    }
  } else {
    if (old_parent && old_parent->type() == TabCollection::Type::TREE_NODE) {
      static_cast<TreeTabNodeTabCollection*>(old_parent)
          ->node_->OnChildHeightChanged({});
    }
  }

  if (!on_move_.is_null()) {
    on_move_.Run(node_->id());
  }
}

}  // namespace tabs
