/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_context_menu_contents.h"

#include "brave/browser/ui/tabs/brave_tab_menu_model.h"
#include "brave/browser/ui/views/tabs/brave_browser_tab_strip_controller.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "components/sessions/core/tab_restore_service.h"
#include "ui/views/controls/menu/menu_runner.h"

BraveTabContextMenuContents::BraveTabContextMenuContents(
    Tab* tab,
    BraveBrowserTabStripController* controller,
    int index)
    : tab_(tab),
      browser_(const_cast<Browser*>(controller->browser())),
      controller_(controller),
      index_(index) {
  model_ = std::make_unique<BraveTabMenuModel>(
      this, controller->model(), index);
  restore_service_ =
      TabRestoreServiceFactory::GetForProfile(browser_->profile());
  menu_runner_ = std::make_unique<views::MenuRunner>(
      model_.get(),
      views::MenuRunner::HAS_MNEMONICS | views::MenuRunner::CONTEXT_MENU);
}

BraveTabContextMenuContents::~BraveTabContextMenuContents() {
}

void BraveTabContextMenuContents::Cancel() {
  controller_ = nullptr;
}

void BraveTabContextMenuContents::RunMenuAt(
    const gfx::Point& point,
    ui::MenuSourceType source_type) {
  menu_runner_->RunMenuAt(tab_->GetWidget(), nullptr,
                          gfx::Rect(point, gfx::Size()),
                          views::MenuAnchorPosition::kTopLeft, source_type);
}

bool BraveTabContextMenuContents::IsCommandIdChecked(int command_id) const {
  return false;
}

bool BraveTabContextMenuContents::IsCommandIdEnabled(int command_id) const {
  if (IsBraveCommandId(command_id))
    return IsBraveCommandIdEnabled(command_id);

  return controller_->IsCommandEnabledForTab(
      static_cast<TabStripModel::ContextMenuCommand>(command_id),
      tab_);
}

bool BraveTabContextMenuContents::GetAcceleratorForCommandId(
    int command_id,
    ui::Accelerator* accelerator) const {
  if (IsBraveCommandId(command_id))
    return false;

  int browser_cmd;
  return TabStripModel::ContextMenuCommandToBrowserCommand(command_id,
                                                           &browser_cmd) ?
      tab_->GetWidget()->GetAccelerator(browser_cmd, accelerator) : false;
}

void BraveTabContextMenuContents::ExecuteCommand(int command_id,
                                                 int event_flags) {
  if (IsBraveCommandId(command_id))
    return ExecuteBraveCommand(command_id);

  // Executing the command destroys |this|, and can also end up destroying
  // |controller_|. So stop the highlights before executing the command.
  controller_->ExecuteCommandForTab(
      static_cast<TabStripModel::ContextMenuCommand>(command_id),
      tab_);
}

bool BraveTabContextMenuContents::IsBraveCommandIdEnabled(
    int command_id) const {
  switch (command_id) {
    case BraveTabMenuModel::CommandRestoreTab:
      return restore_service_ && (!restore_service_->IsLoaded() ||
                                  !restore_service_->entries().empty());
      break;
    case BraveTabMenuModel::CommandBookmarkAllTabs:
      if (browser_) {
        return browser_defaults::bookmarks_enabled &&
               chrome::CanBookmarkAllTabs(browser_);
      }
      break;
    case BraveTabMenuModel::CommandMuteTab:
      return true;
    default:
      NOTREACHED();
      break;
  }

  return false;
}

void BraveTabContextMenuContents::ExecuteBraveCommand(int command_id) {
  switch (command_id) {
    case BraveTabMenuModel::CommandRestoreTab:
      chrome::RestoreTab(browser_);
      return;
    case BraveTabMenuModel::CommandBookmarkAllTabs:
      chrome::BookmarkAllTabs(browser_);
      return;
    case BraveTabMenuModel::CommandMuteTab: {
      const auto* tab_strip_model = browser_->tab_strip_model();
      std::vector<int> affected_indices = tab_strip_model->IsTabSelected(index_)
          ? tab_strip_model->selection_model().selected_indices()
          : std::vector<int>{index_};
      const bool mute = !model_->AreAllTabsMuted(*tab_strip_model, affected_indices);

      for (int tab_index : affected_indices) {
        chrome::SetTabAudioMuted(tab_strip_model->GetWebContentsAt(tab_index),
                                 mute,
                                 TabMutedReason::CONTEXT_MENU,
                                 std::string());
      }
      return;
    }
    default:
      NOTREACHED();
      return;
  }
}

bool BraveTabContextMenuContents::IsBraveCommandId(int command_id) const {
  return command_id > BraveTabMenuModel::CommandStart &&
         command_id < BraveTabMenuModel::CommandLast;
}
