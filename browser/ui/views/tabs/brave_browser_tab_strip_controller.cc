/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_browser_tab_strip_controller.h"

#include <algorithm>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/brave_tab_menu_model_factory.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "brave/browser/ui/tabs/tree_tab_model.h"
#include "brave/browser/ui/views/tabs/brave_tab.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/split_tab_util.h"
#include "chrome/browser/ui/tabs/tab_muted_utils.h"
#include "chrome/browser/ui/views/tabs/browser_tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/core/tab_restore_service.h"
#include "components/split_tabs/split_tab_id.h"
#include "components/tabs/public/tab_interface.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"
#include "ui/events/event.h"
#include "ui/views/view_utils.h"

BraveBrowserTabStripController::BraveBrowserTabStripController(
    TabStripModel* model,
    BrowserView* browser_view,
    std::unique_ptr<TabMenuModelFactory> menu_model_factory_override)
    : BrowserTabStripController(
          model,
          browser_view,
          menu_model_factory_override
              ? std::move(menu_model_factory_override)
              : std::make_unique<brave::BraveTabMenuModelFactory>()) {}

BraveBrowserTabStripController::~BraveBrowserTabStripController() = default;

int BraveBrowserTabStripController::GetTreeHeight(
    const tree_tab::TreeTabNodeId& id) const {
  return static_cast<BraveTabStripModel*>(model_.get())
      ->tree_model()
      ->GetTreeHeight(id);
}

const tabs::TreeTabNode* BraveBrowserTabStripController::GetTreeTabNode(
    const tree_tab::TreeTabNodeId& id) const {
  const auto* node =
      static_cast<BraveTabStripModel*>(model_.get())->tree_model()->GetNode(id);
  return node;
}

void BraveBrowserTabStripController::SetTreeTabNodeCollapsed(
    const tree_tab::TreeTabNodeId& id,
    bool collapsed) {
  static_cast<BraveTabStripModel*>(model_.get())
      ->SetTreeTabNodeCollapsed(id, collapsed);
}

bool BraveBrowserTabStripController::IsInCollapsedTreeTabNode(
    const tree_tab::TreeTabNodeId& id) const {
  return static_cast<BraveTabStripModel*>(model_.get())
      ->tree_model()
      ->DoesBelongToCollapsedNode(id);
}

const tree_tab::TreeTabNodeId*
BraveBrowserTabStripController::GetClosestCollapsedAncestor(
    const tree_tab::TreeTabNodeId& id) const {
  return static_cast<BraveTabStripModel*>(model_.get())
      ->tree_model()
      ->GetClosestCollapsedAncestor(id);
}

const tree_tab::TreeTabNodeId*
BraveBrowserTabStripController::GetTreeTabNodeIdForGroup(
    tab_groups::TabGroupId group_id) const {
  return static_cast<BraveTabStripModel*>(model_.get())
      ->GetTreeTabNodeIdForGroup(group_id);
}

bool BraveBrowserTabStripController::IsCommandEnabledForTab(
    TabStripModel::ContextMenuCommand command_id,
    const Tab* tab) {
  return IsContextMenuCommandEnabled(tab->tab_handle().Get(), command_id);
}

void BraveBrowserTabStripController::ExecuteContextMenuCommand(
    tabs::TabInterface* tab,
    TabStripModel::ContextMenuCommand command_id,
    int event_flags) {
  const int index = model_->GetIndexOfTab(tab);
  if (!model_->ContainsIndex(index)) {
    return;
  }

  // This tab close customization targets only for split |tab|.
  // When select close tab from context menu, we want to close
  // only that split tab instead of both tabs in split.
  // Upstream closes both but we don't want that behavior.
  // As this customization could cause unexpected tab closing
  // behavior, apply strictly in some specific situations.
  // Follow upstream behavior in all other cases.
  // We can add other specific situations when user want to.
  const auto split_id = model_->GetSplitForTab(index);
  if (command_id == TabStripModel::CommandCloseTab && split_id.has_value()) {
    // If |tab| is split and selection size is 1, it means split tab that
    // contains |tab| is inactive and the active tab is normal. Close |tab|.
    if (model_->selection_model().size() == 1) {
      tab->Close();
      return;
    }

    // If only single split tab is activated,
    // selection size is 2 as because upstream puts both tabs
    // in single split view as selected. In this situation, |tab|
    // could be in that active split view or not. Close |tab|.
    const tabs::TabInterface* active_tab = model_->GetActiveTab();
    if (active_tab && active_tab->IsSplit() &&
        model_->selection_model().size() == 2) {
      tab->Close();
      return;
    }
  }

  // Use if clause to prevent enumeration values not handled in switch errors.
  if (command_id == TabStripModel::CommandRestoreTab) {
    chrome::RestoreTab(browser());
    return;
  }

  if (command_id == TabStripModel::CommandBookmarkAllTabs) {
    chrome::BookmarkAllTabs(browser());
    return;
  }

  if (command_id == TabStripModel::CommandShowVerticalTabs) {
    brave::ToggleVerticalTabStrip(browser());
    BrowserView::GetBrowserViewForBrowser(browser())->InvalidateLayout();
    return;
  }

  if (command_id == TabStripModel::CommandToggleTabMuted) {
    auto* model = static_cast<BraveTabStripModel*>(model_.get());
    auto indices = model->GetTabIndicesForCommandAt(index);
    std::vector<content::WebContents*> contentses;
    std::transform(
        indices.begin(), indices.end(), std::back_inserter(contentses),
        [&model](int index) { return model->GetWebContentsAt(index); });

    const auto all_muted = model->GetAllTabsMuted(indices);
    for (auto* contents : contentses) {
      SetTabAudioMuted(contents, !all_muted, TabMutedReason::kAudioIndicator,
                       /*extension_id=*/std::string());
    }
    return;
  }

  if (command_id == TabStripModel::CommandBringAllTabsToThisWindow) {
    brave::BringAllTabs(browser());
    return;
  }

  if (command_id == TabStripModel::CommandCloseDuplicateTabs) {
    brave::CloseDuplicateTabs(browser());
    return;
  }

  BrowserTabStripController::ExecuteContextMenuCommand(tab, command_id,
                                                       event_flags);
}

bool BraveBrowserTabStripController::IsContextMenuCommandChecked(
    TabStripModel::ContextMenuCommand command_id) {
  if (command_id == TabStripModel::CommandShowVerticalTabs) {
    return VerticalTabController::FromBrowser(browser())
        ->ShouldShowBraveVerticalTabs();
  }

  return BrowserTabStripController::IsContextMenuCommandChecked(command_id);
}

bool BraveBrowserTabStripController::IsContextMenuCommandEnabled(
    tabs::TabInterface* tab,
    TabStripModel::ContextMenuCommand command_id) {
  const int index = model_->GetIndexOfTab(tab);
  if (!model_->ContainsIndex(index)) {
    return false;
  }

  // Use if clause to prevent enumeration values not handled in switch errors.
  if (command_id == TabStripModel::CommandRestoreTab) {
    auto* restore_service =
        TabRestoreServiceFactory::GetForProfile(browser()->profile());
    return restore_service && (!restore_service->IsLoaded() ||
                               !restore_service->entries().empty());
  }

  if (command_id == TabStripModel::CommandBookmarkAllTabs) {
    return browser_defaults::bookmarks_enabled &&
           chrome::CanBookmarkAllTabs(browser());
  }

  if (command_id == TabStripModel::CommandToggleTabMuted) {
    auto* model = static_cast<BraveTabStripModel*>(model_.get());
    for (const auto& i : model->GetTabIndicesForCommandAt(index)) {
      if (!model_->GetWebContentsAt(i)->GetLastCommittedURL().is_empty()) {
        return true;
      }
    }
    return false;
  }

  if (command_id == TabStripModel::CommandCloseDuplicateTabs) {
    return brave::HasDuplicateTabs(browser());
  }

  if (command_id == TabStripModel::CommandShowVerticalTabs) {
    return tabs::utils::IsVerticalTabToggleEnabled(browser());
  }

  if (command_id == TabStripModel::CommandBringAllTabsToThisWindow ||
      command_id == TabStripModel::CommandOpenInContainer) {
    return true;
  }

  return BrowserTabStripController::IsContextMenuCommandEnabled(tab,
                                                                command_id);
}

void BraveBrowserTabStripController::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  BrowserTabStripController::OnTabStripModelChanged(tab_strip_model, change,
                                                    selection);

  if (!ShouldShowTreeTabs()) {
    return;
  }

  if (selection.selection_changed()) {
    for (const auto& index : selection.new_model.selected_indices()) {
      auto node = tabstrip_->tab_at(index)->tree_tab_node();
      if (!node) {
        // If newly selected tabs are newly created, they don't have a tree tab
        // node yet, so we skip it here. In this case, OnTreeTabChanged will be
        // called later with kNodeCreated type.
        continue;
      }

      if (IsInCollapsedTreeTabNode(*node)) {
        ExpandAllCollapsedAncestors(*node);
      }
    }
  }
}

void BraveBrowserTabStripController::OnTreeTabChanged(
    const TreeTabChange& change) {
  switch (change.type) {
    case TreeTabChange::Type::kNodeCreated: {
      const auto& created_change = change.GetCreatedChange();
      std::vector<const tabs::TabInterface*> tabs =
          created_change.node->GetTabs();
      for (const tabs::TabInterface* tab : tabs) {
        auto index = model_->GetIndexOfTab(tab);
        // The deferred kNodeCreated notification (via PostTask) can fire
        // while model and tab strip views are out of sync — the tab may
        // have no model index, or the view at that index may not exist yet.
        if (index == TabStripModel::kNoTab ||
            index >= tabstrip_->GetTabCount()) {
          continue;
        }
        auto* tab_view = tabstrip_->tab_at(index);
        tab_view->set_tree_tab_node(change.id);

        if (IsActiveTab(index) && IsInCollapsedTreeTabNode(change.id)) {
          ExpandAllCollapsedAncestors(change.id);
        }
      }
      static_cast<BraveTabStrip*>(tabstrip_.get())
          ->InvalidateTabContainerLayout();
      break;
    }
    case TreeTabChange::Type::kNodeWillBeDestroyed: {
      std::vector<const tabs::TabInterface*> tabs =
          change.GetWillBeDestroyedChange().node->GetTabs();
      for (const tabs::TabInterface* tab : tabs) {
        auto index = model_->GetIndexOfTab(tab);
        // The tab might have already been removed from the model when the
        // TreeTabNode is being destroyed (e.g., during group removal).
        // Or tab could be in detached state during group creation. In this
        // case, the BraeTabStrip::AddTabToGroup() will clear the node id by
        // itself, as this controller have multiple places to clear the node id.
        if (index == TabStripModel::kNoTab) {
          continue;
        }

        auto* tab_view = tabstrip_->tab_at(index);
        tab_view->set_tree_tab_node(std::nullopt);
      }
      static_cast<BraveTabStrip*>(tabstrip_.get())
          ->InvalidateTabContainerLayout();
      break;
    }
    case TreeTabChange::Type::kNodeCollapsedStateChanged: {
      const auto& collapsed_state_changed_change =
          change.GetCollapsedStateChangedChange();
      for (const tabs::TabInterface* tab :
           collapsed_state_changed_change.node->GetTabs()) {
        auto index = model_->GetIndexOfTab(tab);
        CHECK_NE(index, TabStripModel::kNoTab);
        static_cast<BraveTab*>(tabstrip_->tab_at(index))
            ->UpdateTreeToggleButtonIcon();
      }
      static_cast<BraveTabStrip*>(tabstrip_.get())
          ->InvalidateTabContainerLayout();
      break;
    }
    case TreeTabChange::Type::kNodeReparented:
      break;
  }
}

void BraveBrowserTabStripController::SelectTab(int model_index,
                                               const ui::Event& event) {
  auto* brave_model = static_cast<BraveTabStripModel*>(model_.get());
  if (!brave_model->tree_model() ||
      model_index < model_->IndexOfFirstNonPinnedTab()) {
    BrowserTabStripController::SelectTab(model_index, event);
    return;
  }

  // Tab::OnMousePressed() and Tab::OnMouseReleased() both funnel a plain
  // (unmodified) click through SelectTab(): press calls it for a
  // not-yet-selected tab, and release always calls it when no drag occurred,
  // to reset a stale multi-selection down to just the clicked tab. Expanding
  // a tree-tab parent's selection to its whole subtree needs to happen for
  // both, so a subsequent drag (which reads the live selection at press time)
  // carries the subtree along, and the expansion isn't immediately undone by
  // the no-drag reset on release. Other callers of SelectTab() (keyboard,
  // touch, context menu, tests) keep the default single-tab behavior.
  // The possible scenarios are:
  // 1. Click on a not-yet-selected tab: SelectTab() is called TWICE for the
  //    same physical click - once on press (expanding the subtree), and
  //    again on release right afterward (since release always fires when no
  //    drag occurred). Without tree_tab_subtree_expanded_on_press_index_,
  //    that second call can't tell "I'm the tail end of the click
  //    that just expanded this subtree" apart from "I'm a separate, later
  //    click on an already-selected subtree" - both look identical from
  //    here - so it would misread its own press-time expansion as a
  //    deliberate second click and collapse it right back.
  //    - If the user instead starts dragging after the press, only the press
  //      call happens (release's SelectTab() call is skipped when a real
  //      drag completes), so the whole subtree is already selected by the
  //      time the drag reads the live selection.
  // 2. Click on an already-selected tab: press is skipped entirely (Tab::
  //    OnMousePressed() only calls SelectTab() for a not-yet-selected tab),
  //    so only the release call happens, and it toggles the subtree
  //    selection on/off.
  //    - Press on an already-selected tab and then dragging away doesn't
  //      call SelectTab() at all, so the selection is left untouched.
  const bool is_plain_mouse_click =
      event.type() == ui::EventType::kMousePressed ||
      event.type() == ui::EventType::kMouseReleased;

  // Consumed at most once: read whatever the previous call left behind, then
  // clear it so it can't leak into some unrelated later call (e.g. if a real
  // drag intervenes and the paired release never arrives).
  const std::optional<int> expanded_on_previous_press =
      tree_tab_subtree_expanded_on_press_index_;
  tree_tab_subtree_expanded_on_press_index_.reset();

  std::vector<int> descendant_indices;
  bool subtree_already_selected = false;
  int resolved_index = model_index;
  if (is_plain_mouse_click) {
    // Resolve to the tab this call will actually activate (a split's last-
    // active member), matching BrowserTabStripController::SelectTab()'s own
    // redirect, so we consider the right tab's tree-tab descendants.
    if (std::optional<split_tabs::SplitTabId> split_id =
            tabstrip_->tab_at(model_index)->split();
        split_id.has_value()) {
      resolved_index =
          split_tabs::GetIndexOfLastActiveTab(model_, split_id.value());
    }

    descendant_indices =
        brave_model->GetTreeTabDescendantIndices(resolved_index);
    if (!descendant_indices.empty()) {
      if (event.type() == ui::EventType::kMouseReleased &&
          expanded_on_previous_press == resolved_index) {
        // This release is the second half of the same click that already
        // expanded the subtree on press moments ago - not a deliberate,
        // separate second click. Don't treat it as a toggle-off.
        subtree_already_selected = false;
      } else {
        // If the whole subtree is already selected (e.g. a second, unmodified
        // click on a tree node that was already selected together with its
        // descendants), collapse the selection back down to just the clicked
        // tab instead of re-expanding it.
        subtree_already_selected =
            model_->IsTabSelected(resolved_index) &&
            std::ranges::all_of(descendant_indices, [this](int i) {
              return model_->IsTabSelected(i);
            });
      }
    }
  }

  BrowserTabStripController::SelectTab(model_index, event);

  if (descendant_indices.empty() || subtree_already_selected) {
    return;
  }

  absl::flat_hash_set<tabs::TabInterface*> descendant_tabs;
  for (int descendant_index : descendant_indices) {
    descendant_tabs.insert(model_->GetTabAtIndex(descendant_index));
  }

  tabs::TabStripModelSelectionState new_selection = model_->selection_model();
  new_selection.AppendTabsToSelection(std::unordered_set<tabs::TabInterface*>(
      descendant_tabs.begin(), descendant_tabs.end()));
  model_->SetSelectionFromModel(std::move(new_selection));

  if (event.type() == ui::EventType::kMousePressed) {
    tree_tab_subtree_expanded_on_press_index_ = resolved_index;
  }
}

bool BraveBrowserTabStripController::ShouldShowTreeTabs() {
  if (!base::FeatureList::IsEnabled(tabs::kBraveTreeTab)) {
    return false;
  }

  if (!VerticalTabController::FromBrowser(browser())
           ->ShouldShowBraveVerticalTabs()) {
    return false;
  }

  return GetBrowserWindowInterface()->GetProfile()->GetPrefs()->GetBoolean(
      brave_tabs::kTreeTabsEnabled);
}

void BraveBrowserTabStripController::ExpandAllCollapsedAncestors(
    const tree_tab::TreeTabNodeId& id) {
  while (IsInCollapsedTreeTabNode(id)) {
    const auto* collapsed_ancestor = GetClosestCollapsedAncestor(id);
    CHECK(collapsed_ancestor);

    // Note that we need to copy the ancestor ID as |collapsed_ancestor| is
    // going to be invalidated after SetTreeTabNodeCollapsed(false).
    auto target_ancestor = *collapsed_ancestor;
    SetTreeTabNodeCollapsed(target_ancestor, false);
  }
}
