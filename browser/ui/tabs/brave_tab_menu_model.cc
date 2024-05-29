/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_menu_model.h"

#include <algorithm>
#include <vector>

#include "base/feature_list.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_strings.h"
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
  CHECK(web_contents_);
  Browser* browser = chrome::FindBrowserWithTab(web_contents_);
  CHECK(browser);

  restore_service_ =
      TabRestoreServiceFactory::GetForProfile(browser->profile());

  auto indices = static_cast<BraveTabStripModel*>(tab_strip_model)
                     ->GetTabIndicesForCommandAt(index);
  all_muted_ = std::all_of(
      indices.begin(), indices.end(), [&tab_strip_model](int index) {
        return tab_strip_model->GetWebContentsAt(index)->IsAudioMuted();
      });
  Build(browser, tab_strip_model, indices);
}

BraveTabMenuModel::~BraveTabMenuModel() = default;

int BraveTabMenuModel::GetRestoreTabCommandStringId() const {
  int id = IDS_RESTORE_TAB;

  if (!web_contents_) {
    return id;
  }

  if (!restore_service_) {
    return id;
  }

  if (!restore_service_->IsLoaded() || restore_service_->entries().empty()) {
    return id;
  }

  if (restore_service_->entries().front()->type ==
      sessions::tab_restore::WINDOW) {
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

void BraveTabMenuModel::Build(Browser* browser,
                              TabStripModel* tab_strip_model,
                              const std::vector<int>& indices) {
  auto selected_tab_count = indices.size();

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
  AddItemWithStringId(CommandBringAllTabsToThisWindow,
                      IDS_TAB_CXMENU_BRING_ALL_TABS_TO_THIS_WINDOW);

  AddSeparator(ui::NORMAL_SEPARATOR);
  AddCheckItemWithStringId(CommandShowVerticalTabs,
                           IDS_TAB_CXMENU_SHOW_VERTICAL_TABS);

  auto close_other_tabs_index =
      GetIndexOfCommandId(TabStripModel::CommandCloseOtherTabs);
  InsertItemWithStringIdAt(close_other_tabs_index.value_or(GetItemCount()),
                           CommandCloseDuplicateTabs,
                           IDS_TAB_CXMENU_CLOSE_DUPLICATE_TABS);

  if (base::FeatureList::IsEnabled(tabs::features::kBraveSplitView)) {
    BuildItemsForSplitView(browser, tab_strip_model, indices);
    return;
  }
}

void BraveTabMenuModel::BuildItemsForSplitView(
    Browser* browser,
    TabStripModel* tab_strip_model,
    const std::vector<int>& indices) {
  // The split view context menu items are added after the "reload" item.
  auto index = *GetIndexOfCommandId(TabStripModel::CommandReload);

  // In case only one tab is selected
  //  * if the tab is tiled, show "Close Split View" and "Break into Tabs"
  //  * else show "New Split View"
  if (indices.size() == 1u) {
    if (brave::IsTabsTiled(browser, indices)) {
      InsertItemWithStringIdAt(++index, CommandBreakTile, IDS_IDC_BREAK_TILE);
      InsertItemWithStringIdAt(++index, CommandSwapTabsInTile,
                               IDS_IDC_SWAP_SPLIT_VIEW);
      return;
    }

    InsertItemWithStringIdAt(++index, CommandNewSplitView,
                             IDS_IDC_NEW_SPLIT_VIEW);
    return;
  }

  if (brave::CanTileTabs(browser, indices)) {
    InsertItemWithStringIdAt(++index, CommandTileTabs, IDS_IDC_TILE_TABS);
    return;
  }

  if (brave::IsTabsTiled(browser, indices)) {
    InsertItemWithStringIdAt(++index, CommandBreakTile, IDS_IDC_BREAK_TILE);
    InsertItemWithStringIdAt(++index, CommandSwapTabsInTile,
                             IDS_IDC_SWAP_SPLIT_VIEW);
  }
}
