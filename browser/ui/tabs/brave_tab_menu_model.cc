/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_menu_model.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/grit/generated_resources.h"
#include "components/sessions/core/tab_restore_service.h"
#include "ui/base/l10n/l10n_util.h"

BraveTabMenuModel::BraveTabMenuModel(ui::SimpleMenuModel::Delegate* delegate,
                                     TabStripModel* tab_strip_model,
                                     int index)
    : TabMenuModel(delegate, tab_strip_model, index) {
  web_contents_ = tab_strip_model->GetWebContentsAt(index);
  if (web_contents_) {
    Browser* browser = chrome::FindBrowserWithWebContents(web_contents_);
    restore_service_ =
        TabRestoreServiceFactory::GetForProfile(browser->profile());
  }

  Build(tab_strip_model, index);
}

BraveTabMenuModel::~BraveTabMenuModel() {
}

int BraveTabMenuModel::GetRestoreTabCommandStringId() const {
  int id = IDS_RESTORE_TAB;

  if (!web_contents_)
    return id;

  if (!restore_service_)
    return id;

  if (!restore_service_->IsLoaded() || restore_service_->entries().empty())
    return id;

  if (restore_service_->entries().front()->type ==
          sessions::TabRestoreService::WINDOW) {
      id = IDS_RESTORE_WINDOW;
  }

  return id;
}

void BraveTabMenuModel::Build(TabStripModel* tab_strip_model, int index) {
  std::vector<int> affected_indices =
      tab_strip_model->IsTabSelected(index)
          ? tab_strip_model->selection_model().selected_indices()
          : std::vector<int>{index};
  int num_affected_tabs = affected_indices.size();
  const bool will_mute = !AreAllTabsMuted(*tab_strip_model, affected_indices);

  InsertItemAt(GetIndexOfCommandId(TabStripModel::CommandToggleSiteMuted),
               CommandMuteTab,
               will_mute ? l10n_util::GetPluralStringFUTF16(
                              IDS_TAB_CXMENU_MUTE_TAB, num_affected_tabs)
                         : l10n_util::GetPluralStringFUTF16(
                              IDS_TAB_CXMENU_UNMUTE_TAB, num_affected_tabs));

  AddSeparator(ui::NORMAL_SEPARATOR);
  AddItemWithStringId(CommandRestoreTab, GetRestoreTabCommandStringId());
  AddItemWithStringId(CommandBookmarkAllTabs, IDS_TAB_CXMENU_BOOKMARK_ALL_TABS);

}

bool BraveTabMenuModel::AreAllTabsMuted(const TabStripModel& tab_strip_model,
                                        const std::vector<int>& indices) const {
  for (int tab_index : indices) {
    if (!tab_strip_model.GetWebContentsAt(tab_index)->IsAudioMuted())
      return false;
  }
  return true;
}