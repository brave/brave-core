/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/tree_tab_view_utils.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_set>
#include <utility>
#include <variant>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/task/single_thread_task_runner.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/tabs/tree_tab_model.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_selection_state.h"
#include "chrome/browser/ui/tabs/tab_strip_user_gesture_details.h"
#include "chrome/browser/ui/views/tabs/common/split_tab_view.h"
#include "chrome/browser/ui/views/tabs/common/tab_collection_node.h"
#include "chrome/browser/ui/views/tabs/common/tab_group_view.h"
#include "chrome/browser/ui/views/tabs/common/tab_view.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_interface.h"
#include "third_party/abseil-cpp/absl/functional/overload.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/view.h"
#include "ui/views/view_utils.h"

namespace tree_tab_view_utils {

namespace {

// Same as the indentation of Brave's own vertical tab strip.
constexpr int kOffsetPerLevel = 20;

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

// Returns the tree tab node directly wrapping |node_data| (a tab, group or
// split). Returns nullptr if it's not wrapped by a tree tab node.
const tabs::TreeTabNode* GetTreeTabNode(const tabs::ConstChildPtr& node_data) {
  const tabs::TreeTabNodeTabCollection* tree_node_collection = std::visit(
      absl::Overload{
          [](const tabs::ConstDanglingUntriagedTabInterface& tab)
              -> const tabs::TreeTabNodeTabCollection* {
            if (!tab) {
              return nullptr;
            }
            return tabs::TreeTabNodeTabCollection::GetTreeTabNodeCollection(
                tab.get());
          },
          [](const tabs::ConstDanglingUntriagedTabCollection& collection)
              -> const tabs::TreeTabNodeTabCollection* {
            if (!collection) {
              return nullptr;
            }
            const tabs::TabCollection* parent =
                collection->GetParentCollection();
            if (parent &&
                parent->type() == tabs::TabCollection::Type::TREE_NODE) {
              return static_cast<const tabs::TreeTabNodeTabCollection*>(parent);
            }
            return nullptr;
          }},
      node_data);

  return tree_node_collection ? &tree_node_collection->node() : nullptr;
}

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

const tabs::TreeTabNode* GetTreeTabNodeForView(const views::View* view) {
  const TabCollectionNode* node = GetCollectionNodeForView(view);
  if (!node) {
    return nullptr;
  }
  // The node's data can dangle when its collection was already destroyed (e.g.
  // a split that is being removed) but the view hasn't been torn down yet.
  // Resolve collections through their handle, which returns null then.
  const tabs::TabCollectionNodeHandle node_handle = node->GetHandle();
  if (const auto* handle =
          std::get_if<tabs::TabCollectionHandle>(&node_handle)) {
    if (!handle->Get()) {
      return nullptr;
    }
  }
  return GetTreeTabNode(node->GetNodeData());
}

const tabs::TreeTabNode* GetTreeTabNodeForTab(const tabs::TabInterface* tab) {
  return tab ? GetTreeTabNode(tabs::ConstChildPtr(tab)) : nullptr;
}

BraveTabStripModel* GetBraveTabStripModel(tabs::TabInterface* tab) {
  if (!tab) {
    return nullptr;
  }

  BrowserWindowInterface* browser =
      tab ? tab->GetBrowserWindowInterface() : nullptr;
  // Tree tab nodes only exist with BraveTabStripModel.
  return browser ? static_cast<BraveTabStripModel*>(browser->GetTabStripModel())
                 : nullptr;
}

// Adds the tabs at |descendant_indices| to the current selection.
void SelectTreeTabDescendants(BraveTabStripModel& model,
                              const std::vector<int>& descendant_indices) {
  std::unordered_set<tabs::TabInterface*> descendants;
  for (int descendant_index : descendant_indices) {
    descendants.insert(model.GetTabAtIndex(descendant_index));
  }
  tabs::TabStripModelSelectionState new_selection = model.selection_model();
  new_selection.AppendTabsToSelection(descendants);
  model.SetSelectionFromModel(std::move(new_selection));
}

}  // namespace

std::vector<std::reference_wrapper<const tabs::Child>> GetUnwrappedChildren(
    const tabs::TabCollection& collection) {
  std::vector<std::reference_wrapper<const tabs::Child>> children;
  AppendUnwrappedChildren(collection, children);
  return children;
}

int GetTreeTabIndent(
    const views::View* view,
    int available_width,
    absl::flat_hash_map<int, int>& offset_per_level_by_tree_height) {
  const tabs::TreeTabNode* node = GetTreeTabNodeForView(view);
  if (!node || !node->level()) {
    return 0;
  }

  // Keep the tab wider than the collapsed tab strip's tab. When there's not
  // enough room, the levels of the tree share the room evenly.
  const int available_width_for_tree =
      std::max(0, available_width -
                      GetLayoutConstant(LayoutConstant::kVerticalTabMinWidth));
  // The offset only depends on the height of the tree |node| belongs to, so
  // it's computed once per tree for the |available_width| of the caller.
  const int tree_height = node->GetTreeHeight();
  auto [it, inserted] =
      offset_per_level_by_tree_height.try_emplace(tree_height, 0);
  if (inserted) {
    const int tree_levels = std::max(1, tree_height + 1);
    it->second = std::min(kOffsetPerLevel,
                          std::max(1, available_width_for_tree / tree_levels));
  }
  return std::clamp(it->second * node->level(), 0, available_width_for_tree);
}

bool IsInCollapsedTreeTabNode(const views::View* view) {
  const tabs::TreeTabNode* node = GetTreeTabNodeForView(view);
  return node && node->GetClosestCollapsedAncestorId().has_value();
}

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

void ExpandCollapsedTreeTabAncestors(tabs::TabInterface* tab) {
  if (!tab) {
    return;
  }

  // |tab| could be in a split or a group wrapped by a tree tab node.
  tabs::TreeTabNodeTabCollection* tree_node_collection =
      tabs::TreeTabNodeTabCollection::GetNearestTreeTabNodeCollection(tab);
  BraveTabStripModel* model = GetBraveTabStripModel(tab);
  if (!tree_node_collection || !model) {
    return;
  }

  const tabs::TreeTabNode& node = tree_node_collection->node();
  while (auto collapsed_ancestor = node.GetClosestCollapsedAncestorId()) {
    model->SetTreeTabNodeCollapsed(*collapsed_ancestor, false);
    CHECK(node.GetClosestCollapsedAncestorId() != collapsed_ancestor)
        << "Failed to expand.";
  }
}

void ExpandCollapsedTreeTabAncestorsAsync(tabs::TabInterface* tab) {
  if (!tab) {
    return;
  }

  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(
                     [](tabs::TabHandle handle) {
                       // Another tab could've been activated in the meantime.
                       if (auto* tab = handle.Get();
                           tab && tab->IsActivated()) {
                         ExpandCollapsedTreeTabAncestors(tab);
                       }
                     },
                     tab->GetHandle()));
}

void ActivateTabAndSelectTreeTabSubtree(
    TabStripModel& tab_strip_model,
    int index,
    const TabStripUserGestureDetails& detail) {
  auto* model = static_cast<BraveTabStripModel*>(&tab_strip_model);
  if (!model->tree_model()) {
    model->ActivateTabAt(index, detail);
    return;
  }

  // Consumed at most once, so that it can't leak into an unrelated later call
  // (e.g. when a drag intervenes and the paired release never arrives).
  tabs::TabInterface* tab = model->GetTabAtIndex(index);
  const std::optional<tabs::TabHandle> selected_on_press =
      model->TakeTreeTabSubtreeSelectedOnPress();

  // Only a plain mouse click on a tree tab parent selects its subtree.
  std::vector<int> descendant_indices;
  if (detail.type == TabStripUserGestureDetails::GestureType::kMouse) {
    descendant_indices = model->GetTreeTabDescendantIndices(index);
  }

  if (descendant_indices.empty()) {
    model->ActivateTabAt(index, detail);
    return;
  }

  // A mouse press calls this for a not-yet-selected tab, and a mouse release
  // always does when no drag occurred. So this is a press if |tab| isn't
  // selected yet. The selection is expanded on press so that a drag, which
  // reads the selection when it starts, moves the subtree.
  const bool is_press = !model->IsTabSelected(index);

  // The release of the click that expanded the subtree on press isn't a second
  // click, so it must not collapse the subtree. Otherwise, a click on a tab
  // whose whole subtree is already selected collapses the selection to the tab.
  const bool subtree_already_selected =
      !is_press && selected_on_press != tab->GetHandle() &&
      std::ranges::all_of(descendant_indices, [&](int descendant_index) {
        return model->IsTabSelected(descendant_index);
      });

  model->ActivateTabAt(index, detail);
  if (subtree_already_selected) {
    return;
  }

  SelectTreeTabDescendants(*model, descendant_indices);
  if (is_press) {
    model->SetTreeTabSubtreeSelectedOnPress(tab->GetHandle());
  }
}

}  // namespace tree_tab_view_utils
