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
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/brave_tab_menu_model.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/views/tabs/brave_browser_tab_strip_controller.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/tabs/tab_utils.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "components/sessions/core/tab_restore_service.h"
#include "content/public/browser/web_contents.h"
#include "ui/views/controls/menu/menu_runner.h"

namespace {

bool CanTakeTabs(const Browser* from, const Browser* to) {
  return from != to && !from->IsAttemptingToCloseBrowser() &&
         !from->IsBrowserClosing() && !from->is_delete_scheduled() &&
         to->profile() == from->profile();
}

}  // namespace

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
    return base::ranges::any_of(
        *BrowserList::GetInstance(),
        [&](const Browser* from) { return CanTakeTabs(from, browser_); });
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
    case BraveTabMenuModel::CommandShowVerticalTabs:
      return true;
    case BraveTabMenuModel::CommandBringAllTabsToThisWindow: {
      return true;
    }
    default:
      NOTREACHED();
      break;
  }

  return false;
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
        chrome::SetTabAudioMuted(contents, !all_muted,
                                 TabMutedReason::AUDIO_INDICATOR,
                                 /*extension_id=*/std::string());
      }
      return;
    }
    case BraveTabMenuModel::CommandBringAllTabsToThisWindow: {
      BringAllTabsToThisWindow();
      return;
    }
    default:
      NOTREACHED();
      return;
  }
}

void BraveTabContextMenuContents::BringAllTabsToThisWindow() {
  // Find all browsers with the same profile
  std::vector<Browser*> browsers;
  base::ranges::copy_if(
      *BrowserList::GetInstance(), std::back_inserter(browsers),
      [&](const Browser* from) { return CanTakeTabs(from, browser_); });

  // Detach all tabs from other browsers
  std::stack<std::unique_ptr<content::WebContents>> detached_pinned_tabs;
  std::stack<std::unique_ptr<content::WebContents>> detached_unpinned_tabs;

  base::ranges::for_each(browsers, [&detached_pinned_tabs,
                                    &detached_unpinned_tabs](auto* other) {
    auto* tab_strip_model = other->tab_strip_model();
    const int pinned_tab_count = tab_strip_model->IndexOfFirstNonPinnedTab();
    for (int i = tab_strip_model->count() - 1; i >= 0; --i) {
      auto contents = tab_strip_model->DetachWebContentsAtForInsertion(i);
      const bool is_pinned = i < pinned_tab_count;
      if (is_pinned) {
        detached_pinned_tabs.push(std::move(contents));
      } else {
        detached_unpinned_tabs.push(std::move(contents));
      }
    }
  });

  // Insert pinned tabs
  auto* tab_strip_model = browser_->tab_strip_model();
  while (!detached_pinned_tabs.empty()) {
    tab_strip_model->InsertWebContentsAt(
        tab_strip_model->IndexOfFirstNonPinnedTab(),
        std::move(detached_pinned_tabs.top()), AddTabTypes::ADD_PINNED);
    detached_pinned_tabs.pop();
  }

  // Insert unpinned tabs
  while (!detached_unpinned_tabs.empty()) {
    tab_strip_model->InsertWebContentsAt(
        tab_strip_model->count(), std::move(detached_unpinned_tabs.top()),
        AddTabTypes::ADD_NONE);
    detached_unpinned_tabs.pop();
  }
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
