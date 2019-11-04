/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_menu_model.h"

#include "base/auto_reset.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_delegate.h"
#include "chrome/grit/generated_resources.h"
#include "components/sessions/core/tab_restore_service.h"

BraveTabMenuModel::BraveTabMenuModel(ui::SimpleMenuModel::Delegate* delegate,
                                     TabStripModel* tab_strip_model,
                                     int index)
    : TabMenuModel(this, tab_strip_model, index),
      index_(index),
      tab_strip_model_(tab_strip_model),
      delegate_(delegate) {
  content::WebContents* tab = tab_strip_model_->GetWebContentsAt(index_);
  if (tab) {
    browser_ = chrome::FindBrowserWithWebContents(tab);
    restore_service_ =
        TabRestoreServiceFactory::GetForProfile(browser_->profile());
  }

  Build();
}

BraveTabMenuModel::~BraveTabMenuModel() {
}

int BraveTabMenuModel::GetRestoreTabCommandStringId() const {
  int id = IDS_RESTORE_TAB;

  content::WebContents* tab = tab_strip_model_->GetWebContentsAt(index_);
  if (!tab)
    return id;

  auto* browser = chrome::FindBrowserWithWebContents(tab);
  auto* restore_service =
      TabRestoreServiceFactory::GetForProfile(browser->profile());
  if (!restore_service)
    return id;

  if (!restore_service->IsLoaded() || restore_service->entries().empty())
    return id;

  if (restore_service->entries().front()->type ==
          sessions::TabRestoreService::WINDOW)
      id = IDS_RESTORE_WINDOW;

  return id;
}

void BraveTabMenuModel::Build() {
  AddItemWithStringId(CommandCloseOtherTabs, IDS_TAB_CXMENU_CLOSEOTHERTABS);
  AddSeparator(ui::NORMAL_SEPARATOR);
  AddItemWithStringId(CommandRestoreTab, GetRestoreTabCommandStringId());
  AddItemWithStringId(CommandBookmarkAllTabs, IDS_TAB_CXMENU_BOOKMARK_ALL_TABS);
}

bool BraveTabMenuModel::IsCommandIdChecked(int command_id) const {
  if (IsBraveCommandId(command_id))
    return false;

  return delegate_->IsCommandIdChecked(command_id);
}

bool BraveTabMenuModel::IsCommandIdEnabled(int command_id) const {
  if (IsBraveCommandId(command_id))
    return IsBraveCommandIdEnabled(command_id);

  return delegate_->IsCommandIdEnabled(command_id);
}

bool BraveTabMenuModel::GetAcceleratorForCommandId(
    int command_id,
    ui::Accelerator* accelerator) const {
  if (IsBraveCommandId(command_id))
    return false;

  return delegate_->GetAcceleratorForCommandId(command_id, accelerator);
}

void BraveTabMenuModel::ExecuteCommand(int command_id, int event_flags) {
  if (IsBraveCommandId(command_id))
    return ExecuteBraveCommand(command_id);

  delegate_->ExecuteCommand(command_id, event_flags);
}

bool BraveTabMenuModel::IsBraveCommandIdEnabled(int command_id) const {
  switch (command_id) {
    case CommandCloseOtherTabs:
      return !GetIndicesToClosed(index_).empty();
    case CommandRestoreTab:
      return restore_service_ && (!restore_service_->IsLoaded() ||
                                  !restore_service_->entries().empty());
      break;
    case CommandBookmarkAllTabs:
      if (browser_) {
        return browser_defaults::bookmarks_enabled &&
           chrome::CanBookmarkAllTabs(browser_);
      }
      break;
    default:
      NOTREACHED();
      break;
  }

  return false;
}

void BraveTabMenuModel::ExecuteBraveCommand(int command_id) {
  switch (command_id) {
    case CommandCloseOtherTabs: {
      DCHECK(!tab_strip_model_->reentrancy_guard_);
      base::AutoReset<bool> resetter(&tab_strip_model_->reentrancy_guard_,
                                     true);
      tab_strip_model_->InternalCloseTabs(
          tab_strip_model_->GetWebContentsesByIndices(
              GetIndicesToClosed(index_)),
          TabStripModel::CLOSE_CREATE_HISTORICAL_TAB);
      return;
    }
    case CommandRestoreTab:
      chrome::RestoreTab(browser_);
      return;
    case CommandBookmarkAllTabs:
      chrome::BookmarkAllTabs(browser_);
      return;
    default:
      NOTREACHED();
      return;
  }
}

std::vector<int> BraveTabMenuModel::GetIndicesToClosed(int index) const {
  DCHECK(tab_strip_model_->ContainsIndex(index));
  std::vector<int> indices;
  for (int i = tab_strip_model_->count() - 1; i >= 0; --i) {
    if (i != index &&
        !tab_strip_model_->IsTabPinned(i) &&
        !tab_strip_model_->IsTabSelected(i)) {
      indices.push_back(i);
    }
  }
  return indices;
}

bool BraveTabMenuModel::IsBraveCommandId(int command_id) const {
  return command_id > CommandStart && command_id < CommandLast;
}
