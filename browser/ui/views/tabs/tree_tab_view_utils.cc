/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/tree_tab_view_utils.h"

#include <functional>
#include <memory>
#include <optional>
#include <unordered_set>
#include <utility>
#include <variant>

#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "components/tabs/public/tab_collection.h"
#if 0
#include "brave/browser/ui/tabs/tree_tab_model.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_selection_state.h"
#include "chrome/browser/ui/views/tabs/common/split_tab_view.h"
#include "chrome/browser/ui/views/tabs/common/tab_collection_node.h"
#include "chrome/browser/ui/views/tabs/common/tab_group_view.h"
#include "chrome/browser/ui/views/tabs/common/tab_view.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_interface.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/view.h"
#include "ui/views/view_utils.h"
#endif

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

void AppendUnwrappedChildren(
    const tabs::TabCollection& collection,
    std::vector<std::reference_wrapper<const tabs::Child>>& out) {
  for (const auto& child : collection.GetChildren()) {
    const auto* child_collection =
        std::get_if<std::unique_ptr<tabs::TabCollection>>(&child);
    if (child_collection &&
        (*child_collection)->type() == tabs::TabCollection::Type::TREE_NODE) {
      AppendUnwrappedChildren(**child_collection, out);
    } else {
      out.push_back(std::cref(child));
    }
  }
}

#if 0
// Returns the parent of |item| in the collection hierarchy.
const tabs::TabCollection* GetRawParent(const tabs::ConstChildPtr& item) {
  if (const auto* tab =
          std::get_if<tabs::ConstDanglingUntriagedTabInterface>(&item)) {
    return tab->get() ? tab->get()->GetParentCollection() : nullptr;
  }
  const tabs::TabCollection* collection =
      std::get<tabs::ConstDanglingUntriagedTabCollection>(item);
  return collection ? collection->GetParentCollection() : nullptr;
}

std::optional<size_t> GetRawIndex(const tabs::TabCollection& parent,
                                  const tabs::ConstChildPtr& item) {
  const auto& children = parent.GetChildren();
  for (size_t i = 0; i < children.size(); ++i) {
    if (const auto* child_collection =
            std::get_if<std::unique_ptr<tabs::TabCollection>>(&children[i])) {
      const auto* collection =
          std::get_if<tabs::ConstDanglingUntriagedTabCollection>(&item);
      if (collection && collection->get() == child_collection->get()) {
        return i;
      }
    } else {
      const auto* tab =
          std::get_if<tabs::ConstDanglingUntriagedTabInterface>(&item);
      if (tab &&
          tab->get() == static_cast<const tabs::TabInterface*>(
                            std::get<tabs::ScopedTab>(children[i]).get())) {
        return i;
      }
    }
  }
  return std::nullopt;
}

std::optional<tabs::ConstChildPtr> GetLastItemForView(
    const tabs::TabCollection& collection);

// Returns the last item that |child| is mirrored as in the view model.
template <typename Child>
std::optional<tabs::ConstChildPtr> GetLastItemOfChild(const Child& child) {
  if (const auto* child_collection =
          std::get_if<std::unique_ptr<tabs::TabCollection>>(&child)) {
    const tabs::TabCollection* ptr = child_collection->get();
    if (ptr->type() == tabs::TabCollection::Type::TREE_NODE) {
      return GetLastItemForView(*ptr);
    }
    return tabs::ConstChildPtr(ptr);
  }
  return tabs::ConstChildPtr(static_cast<const tabs::TabInterface*>(
      std::get<tabs::ScopedTab>(child).get()));
}

std::optional<tabs::ConstChildPtr> GetLastItemForView(
    const tabs::TabCollection& collection) {
  const auto& children = collection.GetChildren();
  for (size_t i = children.size(); i > 0; --i) {
    if (auto item = GetLastItemOfChild(children[i - 1])) {
      return item;
    }
  }
  return std::nullopt;
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

#endif
const tabs::TreeTabNode* GetTreeTabNodeForTab(const tabs::TabInterface* tab) {
  return tab ? GetTreeTabNode(tabs::ConstChildPtr(tab)) : nullptr;
}

// Tree tab nodes only exist with BraveTabStripModel.
BraveTabStripModel* GetBraveTabStripModel(const tabs::TabInterface* tab) {
  BrowserWindowInterface* browser =
      tab ? const_cast<tabs::TabInterface*>(tab)->GetBrowserWindowInterface()
          : nullptr;
  return browser ? static_cast<BraveTabStripModel*>(browser->GetTabStripModel())
                 : nullptr;
}

}  // namespace

std::vector<std::reference_wrapper<const tabs::Child>> GetUnwrappedChildren(
    const tabs::TabCollection& collection) {
  std::vector<std::reference_wrapper<const tabs::Child>> children;
  AppendUnwrappedChildren(collection, children);
  return children;
}

#if 0
const tabs::TabCollection* GetUnwrappedParent(const tabs::ConstChildPtr& item) {
  const tabs::TabCollection* parent = GetRawParent(item);
  while (parent && parent->type() == tabs::TabCollection::Type::TREE_NODE) {
    parent = parent->GetParentCollection();
  }
  return parent;
}

std::optional<tabs::ConstChildPtr> GetUnwrappedPredecessor(
    const tabs::ConstChildPtr& item) {
  tabs::ConstChildPtr current = item;
  while (const tabs::TabCollection* parent = GetRawParent(current)) {
    const std::optional<size_t> index = GetRawIndex(*parent, current);
    if (!index) {
      return std::nullopt;
    }

    const auto& children = parent->GetChildren();
    for (size_t i = *index; i > 0; --i) {
      if (auto previous = GetLastItemOfChild(children[i - 1])) {
        return previous;
      }
    }

    if (parent->type() != tabs::TabCollection::Type::TREE_NODE) {
      return std::nullopt;
    }
    // The tree tab node is unwrapped. What comes before it comes before |item|
    // as well.
    current =
        tabs::ConstChildPtr(static_cast<const tabs::TabCollection*>(parent));
  }
  return std::nullopt;
}

bool IsInTabStrip(const tabs::TabCollection* root,
                  const tabs::ConstChildPtr& item) {
  for (const tabs::TabCollection* parent = GetRawParent(item); parent;
       parent = parent->GetParentCollection()) {
    if (parent == root) {
      return true;
    }
  }
  return false;
}

std::vector<const tabs::TabInterface*> GetTabsOfTreeTabChange(
    TabStripModel* model,
    const TreeTabChange& change) {
  if (change.type == TreeTabChange::kNodeWillBeDestroyed) {
    return {};
  }
  TreeTabModel* tree_model =
      static_cast<BraveTabStripModel*>(model)->tree_model();
  const tabs::TreeTabNode* node =
      tree_model ? tree_model->GetNode(change.id) : nullptr;
  return node ? node->GetTabs() : std::vector<const tabs::TabInterface*>();
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

#endif  // 0

bool HasTreeTabDescendants(const tabs::TabInterface* tab) {
  const tabs::TreeTabNode* node = GetTreeTabNodeForTab(tab);
  return node && node->height() > 0;
}

bool IsTreeTabNodeCollapsed(const tabs::TabInterface* tab) {
  const tabs::TreeTabNode* node = GetTreeTabNodeForTab(tab);
  return node && node->collapsed();
}

std::u16string GetTreeTabToggleButtonLabel(const tabs::TabInterface* tab) {
  return l10n_util::GetStringUTF16(IsTreeTabNodeCollapsed(tab)
                                       ? IDS_TREE_TAB_EXPAND_BUTTON
                                       : IDS_TREE_TAB_COLLAPSE_BUTTON);
}

void ToggleTreeTabNodeCollapsed(tabs::TabInterface* tab) {
  const tabs::TreeTabNode* node = GetTreeTabNodeForTab(tab);
  BraveTabStripModel* model = GetBraveTabStripModel(tab);
  if (!node || !model) {
    return;
  }
  model->SetTreeTabNodeCollapsed(node->id(), !node->collapsed());
}

void AddTreeTabDescendantsToSelection(tabs::TabInterface* tab) {
  BraveTabStripModel* model = GetBraveTabStripModel(tab);
  if (!model || !GetTreeTabNodeForTab(tab)) {
    return;
  }

  // Only when |tab| is selected, e.g. not when the press deselected it.
  const int index = model->GetIndexOfTab(tab);
  if (index == TabStripModel::kNoTab || !model->IsTabSelected(index)) {
    return;
  }

  std::unordered_set<tabs::TabInterface*> descendants;
  for (int descendant_index : model->GetTreeTabDescendantIndices(index)) {
    descendants.insert(model->GetTabAtIndex(descendant_index));
  }
  if (descendants.empty()) {
    return;
  }

  tabs::TabStripModelSelectionState new_selection = model->selection_model();
  new_selection.AppendTabsToSelection(descendants);
  model->SetSelectionFromModel(std::move(new_selection));
}

}  // namespace tree_tab_view_utils
