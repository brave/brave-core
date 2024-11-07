/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_context_menu_contents.h"

#include <algorithm>
#include <iterator>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/notimplemented.h"
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/brave_tab_menu_model.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/browser/ui/views/tabs/brave_browser_tab_strip_controller.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/tabs/tab_utils.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "components/sessions/core/tab_restore_service.h"
#include "content/public/browser/web_contents.h"
#include "ui/views/controls/menu/menu_runner.h"

BraveTabContextMenuContents::BraveTabContextMenuContents(
    Tab* tab,
    BraveBrowserTabStripController* controller,
    int index)
    : tab_(tab),
      tab_index_(index),
      browser_(const_cast<Browser*>(controller->browser())),
      controller_(controller) {
  const bool is_vertical_tab = tabs::utils::ShouldShowVerticalTabs(browser_);

  model_ = std::make_unique<BraveTabMenuModel>(
      this, controller->browser()->tab_menu_model_delegate(),
      controller->model(), index, is_vertical_tab);
  restore_service_ =
      TabRestoreServiceFactory::GetForProfile(browser_->profile());
  menu_runner_ = std::make_unique<views::MenuRunner>(
      model_.get(),
      views::MenuRunner::HAS_MNEMONICS | views::MenuRunner::CONTEXT_MENU,
      base::BindRepeating(&BraveTabContextMenuContents::OnMenuClosed,
                          weak_ptr_.GetWeakPtr()));
}

BraveTabContextMenuContents::~BraveTabContextMenuContents() = default;

void BraveTabContextMenuContents::Cancel() {
  controller_ = nullptr;
}

void BraveTabContextMenuContents::RunMenuAt(const gfx::Point& point,
                                            ui::MenuSourceType source_type) {
  menu_runner_->RunMenuAt(tab_->GetWidget(), nullptr,
                          gfx::Rect(point, gfx::Size()),
                          views::MenuAnchorPosition::kTopLeft, source_type);
}

bool BraveTabContextMenuContents::IsCommandIdChecked(int command_id) const {
  if (!IsValidContextMenu()) {
    return false;
  }

  if (command_id == BraveTabMenuModel::CommandShowVerticalTabs) {
    return tabs::utils::ShouldShowVerticalTabs(browser_);
  }

  return ui::SimpleMenuModel::Delegate::IsCommandIdChecked(command_id);
}

bool BraveTabContextMenuContents::IsCommandIdEnabled(int command_id) const {
  // This could be called after tab is closed.
  if (!IsValidContextMenu()) {
    return false;
  }

  if (IsBraveCommandId(command_id)) {
    return IsBraveCommandIdEnabled(command_id);
  }

  return controller_->IsCommandEnabledForTab(
      static_cast<TabStripModel::ContextMenuCommand>(command_id), tab_);
}

bool BraveTabContextMenuContents::IsCommandIdVisible(int command_id) const {
  if (!IsValidContextMenu()) {
    return false;
  }

  if (command_id == BraveTabMenuModel::CommandShowVerticalTabs) {
    return tabs::utils::SupportsVerticalTabs(browser_);
  }

  if (command_id == BraveTabMenuModel::CommandBringAllTabsToThisWindow) {
    return brave::CanBringAllTabs(browser_);
  }

  return ui::SimpleMenuModel::Delegate::IsCommandIdVisible(command_id);
}

bool BraveTabContextMenuContents::GetAcceleratorForCommandId(
    int command_id,
    ui::Accelerator* accelerator) const {
  if (!IsValidContextMenu()) {
    return false;
  }

  if (IsBraveCommandId(command_id)) {
    return false;
  }

  int browser_cmd;
  views::Widget* widget =
      BrowserView::GetBrowserViewForBrowser(browser_)->GetWidget();
  return TabStripModel::ContextMenuCommandToBrowserCommand(command_id,
                                                           &browser_cmd) &&
         widget->GetAccelerator(browser_cmd, accelerator);
}

void BraveTabContextMenuContents::ExecuteCommand(int command_id,
                                                 int event_flags) {
  if (!IsValidContextMenu()) {
    return;
  }

  if (IsBraveCommandId(command_id)) {
    return ExecuteBraveCommand(command_id);
  }

  // Executing the command destroys |this|, and can also end up destroying
  // |controller_|. So stop the highlights before executing the command.
  controller_->ExecuteCommandForTab(
      static_cast<TabStripModel::ContextMenuCommand>(command_id), tab_);
}

bool BraveTabContextMenuContents::IsBraveCommandIdEnabled(
    int command_id) const {
  CHECK(IsValidContextMenu());

  switch (command_id) {
    case BraveTabMenuModel::CommandRestoreTab:
      return restore_service_ && (!restore_service_->IsLoaded() ||
                                  !restore_service_->entries().empty());
    case BraveTabMenuModel::CommandBookmarkAllTabs:
      if (browser_) {
        return browser_defaults::bookmarks_enabled &&
               chrome::CanBookmarkAllTabs(browser_);
      }
      break;
    case BraveTabMenuModel::CommandToggleTabMuted: {
      auto* model = static_cast<BraveTabStripModel*>(controller_->model());
      for (const auto& index : model->GetTabIndicesForCommandAt(tab_index_)) {
        if (!model->GetWebContentsAt(index)->GetLastCommittedURL().is_empty()) {
          return true;
        }
      }
      return false;
    }
    case BraveTabMenuModel::CommandCloseDuplicateTabs:
      return brave::HasDuplicateTabs(browser_);
    case BraveTabMenuModel::CommandShowVerticalTabs:
      [[fallthrough]];
    case BraveTabMenuModel::CommandBringAllTabsToThisWindow:
      [[fallthrough]];
    case BraveTabMenuModel::CommandNewSplitView:
      [[fallthrough]];
    case BraveTabMenuModel::CommandTileTabs:
      [[fallthrough]];
    case BraveTabMenuModel::CommandBreakTile:
      [[fallthrough]];
    case BraveTabMenuModel::CommandSwapTabsInTile:
      return true;
    default:
      break;
  }
  NOTREACHED() << "All commands are handled above";
}

void BraveTabContextMenuContents::ExecuteBraveCommand(int command_id) {
  CHECK(IsValidContextMenu());

  switch (command_id) {
    case BraveTabMenuModel::CommandRestoreTab:
      chrome::RestoreTab(browser_);
      return;
    case BraveTabMenuModel::CommandBookmarkAllTabs:
      chrome::BookmarkAllTabs(browser_);
      return;
    case BraveTabMenuModel::CommandShowVerticalTabs: {
      brave::ToggleVerticalTabStrip(browser_);
      BrowserView::GetBrowserViewForBrowser(browser_)->InvalidateLayout();
      return;
    }
    case BraveTabMenuModel::CommandToggleTabMuted: {
      auto* model = static_cast<BraveTabStripModel*>(controller_->model());
      auto indices = model->GetTabIndicesForCommandAt(tab_index_);
      std::vector<content::WebContents*> contentses;
      std::transform(
          indices.begin(), indices.end(), std::back_inserter(contentses),
          [&model](int index) { return model->GetWebContentsAt(index); });

      auto all_muted = model_->all_muted();
      for (auto* contents : contentses) {
        SetTabAudioMuted(contents, !all_muted, TabMutedReason::AUDIO_INDICATOR,
                         /*extension_id=*/std::string());
      }
      return;
    }
    case BraveTabMenuModel::CommandBringAllTabsToThisWindow: {
      brave::BringAllTabs(browser_);
      return;
    }
    case BraveTabMenuModel::CommandCloseDuplicateTabs:
      brave::CloseDuplicateTabs(browser_);
      return;
    case BraveTabMenuModel::CommandNewSplitView:
      NewSplitView();
      return;
    case BraveTabMenuModel::CommandTileTabs:
      TileSelectedTabs();
      return;
    case BraveTabMenuModel::CommandBreakTile:
      BreakSelectedTile();
      return;
    case BraveTabMenuModel::CommandSwapTabsInTile:
      SwapTabsInTile();
      return;
    default:
      break;
  }
  NOTREACHED() << "All commands are handled above";
}

bool BraveTabContextMenuContents::IsBraveCommandId(int command_id) const {
  return command_id > BraveTabMenuModel::CommandStart &&
         command_id < BraveTabMenuModel::CommandLast;
}

bool BraveTabContextMenuContents::IsValidContextMenu() const {
  if (menu_closed_) {
    return false;
  }

  return controller_->GetModelIndexOf(tab_).has_value() &&
         controller_->model()->ContainsIndex(tab_index_);
}

void BraveTabContextMenuContents::OnMenuClosed() {
  menu_closed_ = true;
}

void BraveTabContextMenuContents::NewSplitView() {
  auto* model = browser_->tab_strip_model();
  auto tab = model->GetTabHandleAt(tab_index_);
  brave::NewSplitViewForTab(browser_, tab);
}

void BraveTabContextMenuContents::TileSelectedTabs() {
  brave::TileTabs(browser_, GetTabIndicesForSplitViewCommand());
}

void BraveTabContextMenuContents::BreakSelectedTile() {
  brave::BreakTiles(browser_, GetTabIndicesForSplitViewCommand());
}

void BraveTabContextMenuContents::SwapTabsInTile() {
  brave::SwapTabsInTile(browser_);
}

std::vector<int> BraveTabContextMenuContents::GetTabIndicesForSplitViewCommand()
    const {
  auto* model = static_cast<BraveTabStripModel*>(controller_->model());
  auto selected_indices = model->GetTabIndicesForCommandAt(tab_index_);
  if (base::Contains(selected_indices, tab_index_)) {
    return selected_indices;
  }

  return {tab_index_};
}
