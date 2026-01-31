/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_browser_tab_strip_controller.h"

#include <utility>

#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/brave_tab_menu_model_factory.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/tabs/tree_tab_model.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_muted_utils.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "components/sessions/core/tab_restore_service.h"
#include "components/tabs/public/tab_interface.h"

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

void BraveBrowserTabStripController::EnterTabRenameModeAt(int index) {
  CHECK(base::FeatureList::IsEnabled(tabs::kBraveRenamingTabs));
  return static_cast<BraveTabStrip*>(tabstrip_)->EnterTabRenameModeAt(index);
}

void BraveBrowserTabStripController::SetCustomTitleForTab(
    int index,
    const std::optional<std::u16string>& title) {
  static_cast<BraveTabStripModel*>(model_.get())
      ->SetCustomTitleForTab(index, title);
}

bool BraveBrowserTabStripController::IsCommandEnabledForTab(
    TabStripModel::ContextMenuCommand command_id,
    const Tab* tab) {
  const std::optional<int> model_index = tabstrip_->GetModelIndexOf(tab);
  return model_index.has_value()
             ? IsContextMenuCommandEnabled(model_index.value(), command_id)
             : false;
}

void BraveBrowserTabStripController::ExecuteContextMenuCommand(
    int index,
    TabStripModel::ContextMenuCommand command_id,
    int event_flags) {
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
    auto* tab_interface = model_->GetTabAtIndex(index);
    // If |tab| is split and selection size is 1, it means split tab that
    // contains |tab| is inactive and the active tab is normal. Close |tab|.
    if (model_->selection_model().size() == 1) {
      tab_interface->Close();
      return;
    }

    // If only single split tab is activated,
    // selection size is 2 as because upstream puts both tabs
    // in single split view as selected. In this situation, |tab|
    // could be in that active split view or not. Close |tab|.
    if (model_->IsActiveTabSplit() && model_->selection_model().size() == 2) {
      tab_interface->Close();
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

  if (command_id == TabStripModel::CommandRenameTab) {
    EnterTabRenameModeAt(index);
    return;
  }

  BrowserTabStripController::ExecuteContextMenuCommand(index, command_id,
                                                       event_flags);
}

bool BraveBrowserTabStripController::IsContextMenuCommandChecked(
    TabStripModel::ContextMenuCommand command_id) {
  if (command_id == TabStripModel::CommandShowVerticalTabs) {
    return tabs::utils::ShouldShowBraveVerticalTabs(browser());
  }

  return BrowserTabStripController::IsContextMenuCommandChecked(command_id);
}

bool BraveBrowserTabStripController::IsContextMenuCommandEnabled(
    int index,
    TabStripModel::ContextMenuCommand command_id) {
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

  if (command_id == TabStripModel::CommandShowVerticalTabs ||
      command_id == TabStripModel::CommandBringAllTabsToThisWindow ||
      command_id == TabStripModel::CommandOpenInContainer ||
      command_id == TabStripModel::CommandRenameTab) {
    return true;
  }

  return BrowserTabStripController::IsContextMenuCommandEnabled(index,
                                                                command_id);
}

void BraveBrowserTabStripController::OnTreeTabChanged(
    const TreeTabChange& change) {
  switch (change.type) {
    case TreeTabChange::Type::kNodeCreated: {
      const auto& created_change = change.GetCreatedChange();
      auto index = model_->GetIndexOfTab(created_change.node->GetTab());
      CHECK_NE(index, TabStripModel::kNoTab);
      tabstrip_->tab_at(index)->set_tree_tab_node(change.id);
      break;
    }
    case TreeTabChange::Type::kNodeWillBeDestroyed: {
      auto* tab = change.GetWillBeDestroyedChange().node->GetTab();
      CHECK(tab);
      auto index = model_->GetIndexOfTab(tab);
      // The tab might have already been removed from the model when the
      // TreeTabNode is being destroyed (e.g., during group removal).
      if (index != TabStripModel::kNoTab) {
        tabstrip_->tab_at(index)->set_tree_tab_node(std::nullopt);
      }
      break;
    }
  }
}
