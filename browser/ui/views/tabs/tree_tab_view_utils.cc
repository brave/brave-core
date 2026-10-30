/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/tree_tab_view_utils.h"

#include <algorithm>
#include <memory>
#include <variant>

#include "brave/components/tabs/public/tree_tab_node.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/tabs/common/split_tab_view.h"
#include "chrome/browser/ui/views/tabs/common/tab_collection_node.h"
#include "chrome/browser/ui/views/tabs/common/tab_group_view.h"
#include "chrome/browser/ui/views/tabs/common/tab_view.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_interface.h"
#include "ui/views/view.h"
#include "ui/views/view_utils.h"

namespace tree_tab_view_utils {

namespace {

// Same as the indentation of Brave's own vertical tab strip.
constexpr int kOffsetPerLevel = 20;

const TabCollectionNode* GetCollectionNodeForView(const views::View* view) {
  if (const auto* tab_view = views::AsViewClass<TabView>(view)) {
    return tab_view->collection_node();
  }
  if (const auto* group_view = views::AsViewClass<TabGroupView>(view)) {
    return group_view->collection_node();
  }
  if (const auto* split_view = views::AsViewClass<SplitTabView>(view)) {
    return split_view->collection_node();
  }
  return nullptr;
}

void AppendChildrenForView(const tabs::TabCollection& collection,
                           tabs::ConstChildrenPtrs& out) {
  for (const auto& child : collection.GetChildren()) {
    if (const auto* child_collection =
            std::get_if<std::unique_ptr<tabs::TabCollection>>(&child)) {
      const tabs::TabCollection* ptr = child_collection->get();
      if (ptr->type() == tabs::TabCollection::Type::TREE_NODE) {
        AppendChildrenForView(*ptr, out);
      } else {
        out.emplace_back(ptr);
      }
    } else {
      out.emplace_back(static_cast<const tabs::TabInterface*>(
          std::get<tabs::ScopedTab>(child).get()));
    }
  }
}

// Returns the tree tab node directly wrapping |node_data| (a tab, group or
// split). Returns nullptr if it's not wrapped by a tree tab node.
const tabs::TreeTabNode* GetTreeTabNode(const tabs::ConstChildPtr& node_data) {
  const tabs::TreeTabNodeTabCollection* tree_node_collection = nullptr;
  if (const auto* tab =
          std::get_if<tabs::ConstDanglingUntriagedTabInterface>(&node_data)) {
    if (!*tab) {
      return nullptr;
    }
    tree_node_collection =
        tabs::TreeTabNodeTabCollection::GetTreeTabNodeCollection(tab->get());
  } else {
    const tabs::TabCollection* collection =
        std::get<tabs::ConstDanglingUntriagedTabCollection>(node_data);
    if (!collection) {
      return nullptr;
    }
    const tabs::TabCollection* parent = collection->GetParentCollection();
    if (parent && parent->type() == tabs::TabCollection::Type::TREE_NODE) {
      tree_node_collection =
          static_cast<const tabs::TreeTabNodeTabCollection*>(parent);
    }
  }

  return tree_node_collection ? &tree_node_collection->node() : nullptr;
}

const tabs::TreeTabNode* GetTreeTabNodeForView(const views::View* view) {
  const TabCollectionNode* node = GetCollectionNodeForView(view);
  return node ? GetTreeTabNode(node->GetNodeData()) : nullptr;
}

}  // namespace

tabs::ConstChildrenPtrs GetChildrenForView(
    const tabs::TabCollection& collection) {
  tabs::ConstChildrenPtrs children;
  AppendChildrenForView(collection, children);
  return children;
}

bool HasTreeTabNodeChild(const tabs::TabCollection& collection) {
  return std::ranges::any_of(collection.GetChildren(), [](const auto& child) {
    const auto* child_collection =
        std::get_if<std::unique_ptr<tabs::TabCollection>>(&child);
    return child_collection &&
           (*child_collection)->type() == tabs::TabCollection::Type::TREE_NODE;
  });
}

int GetTreeTabIndent(const views::View* view, int available_width) {
  const tabs::TreeTabNode* node = GetTreeTabNodeForView(view);
  if (!node || !node->level()) {
    return 0;
  }

  // Keep the tab wider than the collapsed tab strip's tab. When there's not
  // enough room, the levels of the tree share the room evenly.
  const int available_width_for_tree =
      std::max(0, available_width -
                      GetLayoutConstant(LayoutConstant::kVerticalTabMinWidth));
  const int tree_levels = std::max(1, node->GetTreeHeight() + 1);
  const int offset_per_level = std::min(
      kOffsetPerLevel, std::max(1, available_width_for_tree / tree_levels));
  return std::clamp(offset_per_level * node->level(), 0,
                    available_width_for_tree);
}

bool IsInCollapsedTreeTabNode(const views::View* view) {
  const tabs::TreeTabNode* node = GetTreeTabNodeForView(view);
  return node && node->GetClosestCollapsedAncestorId().has_value();
}

}  // namespace tree_tab_view_utils
