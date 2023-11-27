/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_menu_model.h"

#include <algorithm>
#include <vector>

#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/grit/generated_resources.h"
#include "components/sessions/core/tab_restore_service.h"
#include "ui/base/l10n/l10n_util.h"

BraveTabMenuModel::BraveTabMenuModel(
    ui::SimpleMenuModel::Delegate* delegate,
    TabMenuModelDelegate* tab_menu_model_delegate,
    TabStripModel* tab_strip_model,
    int index,
    bool is_vertical_tab)
    : TabMenuModel(delegate, tab_menu_model_delegate, tab_strip_model, index),
      is_vertical_tab_(is_vertical_tab) {
  web_contents_ = tab_strip_model->GetWebContentsAt(index);
  if (web_contents_) {
    Browser* browser = chrome::FindBrowserWithTab(web_contents_);
    restore_service_ =
        TabRestoreServiceFactory::GetForProfile(browser->profile());
  }

  auto indices = static_cast<BraveTabStripModel*>(tab_strip_model)
                     ->GetTabIndicesForCommandAt(index);
  all_muted_ = std::all_of(
      indices.begin(), indices.end(), [&tab_strip_model](int index) {
        return tab_strip_model->GetWebContentsAt(index)->IsAudioMuted();
      });
  Build(indices.size());
}

BraveTabMenuModel::~BraveTabMenuModel() = default;

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

std::u16string BraveTabMenuModel::GetLabelAt(size_t index) const {
  if (!is_vertical_tab_) {
    return TabMenuModel::GetLabelAt(index);
  }

  if (auto command_id = GetCommandIdAt(index);
      command_id == TabStripModel::CommandNewTabToRight) {
    return l10n_util::GetStringUTF16(
        IDS_TAB_CXMENU_NEWTABTORIGHT_VERTICAL_TABS);
  } else if (command_id == TabStripModel::CommandCloseTabsToRight) {
    return l10n_util::GetStringUTF16(
        IDS_TAB_CXMENU_CLOSETABSTORIGHT_VERTICAL_TABS);
  }

  return TabMenuModel::GetLabelAt(index);
}

void BraveTabMenuModel::Build(int selected_tab_count) {
  AddSeparator(ui::NORMAL_SEPARATOR);
  auto mute_site_index =
      GetIndexOfCommandId(TabStripModel::CommandToggleSiteMuted);

  auto toggle_tab_mute_label = l10n_util::GetPluralStringFUTF16(
      all_muted() ? IDS_TAB_CXMENU_SOUND_UNMUTE_TAB
                  : IDS_TAB_CXMENU_SOUND_MUTE_TAB,
      selected_tab_count);
  InsertItemAt(mute_site_index.value_or(GetItemCount()), CommandToggleTabMuted,
               toggle_tab_mute_label);

  AddItemWithStringId(CommandRestoreTab, GetRestoreTabCommandStringId());
  AddItemWithStringId(CommandBookmarkAllTabs, IDS_TAB_CXMENU_BOOKMARK_ALL_TABS);

  AddSeparator(ui::NORMAL_SEPARATOR);
  AddCheckItemWithStringId(CommandShowVerticalTabs,
                           IDS_TAB_CXMENU_SHOW_VERTICAL_TABS);
}
