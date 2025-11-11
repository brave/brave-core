/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_menu_model.h"

#include <algorithm>
#include <vector>

#include "base/check.h"
#include "base/feature_list.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/brave_split_tab_menu_model.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "chrome/browser/ui/tabs/features.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/sessions/core/tab_restore_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/menus/simple_menu_model.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/core/browser/prefs.h"
#include "brave/components/containers/core/common/features.h"
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

BraveTabMenuModel::BraveTabMenuModel(
    ui::SimpleMenuModel::Delegate* delegate,
    TabMenuModelDelegate* tab_menu_model_delegate,
    TabStripModel* tab_strip_model,
#if BUILDFLAG(ENABLE_CONTAINERS)
    containers::ContainersMenuModel::Delegate& containers_delegate,
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
    int index,
    bool is_vertical_tab)
    : TabMenuModel(delegate, tab_menu_model_delegate, tab_strip_model, index),
      is_vertical_tab_(is_vertical_tab)
#if BUILDFLAG(ENABLE_CONTAINERS)
      ,
      containers_menu_model_delegate_(containers_delegate)
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
{
  auto* web_contents = tab_strip_model->GetWebContentsAt(index);
  CHECK(web_contents);
  Browser* browser = chrome::FindBrowserWithTab(web_contents);
  CHECK(browser);

  restore_service_ =
      TabRestoreServiceFactory::GetForProfile(browser->profile());

  auto indices = static_cast<BraveTabStripModel*>(tab_strip_model)
                     ->GetTabIndicesForCommandAt(index);
  all_muted_ = std::all_of(
      indices.begin(), indices.end(), [&tab_strip_model](int index) {
        return tab_strip_model->GetWebContentsAt(index)->IsAudioMuted();
      });
  Build(browser, tab_strip_model, index, indices);
}

BraveTabMenuModel::~BraveTabMenuModel() = default;

int BraveTabMenuModel::GetRestoreTabCommandStringId() const {
  int id = IDS_RESTORE_TAB;

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

bool BraveTabMenuModel::IsNewFeatureAt(size_t index) const {
  if (base::FeatureList::IsEnabled(features::kSideBySide)) {
    const auto id = GetCommandIdAt(index);

    // Don't show new badge for split view commands.
    if (id == TabStripModel::CommandSwapWithActiveSplit ||
        id == TabStripModel::CommandAddToSplit ||
        id == TabStripModel::CommandArrangeSplit) {
      return false;
    }
  }

  return TabMenuModel::IsNewFeatureAt(index);
}

void BraveTabMenuModel::Build(Browser* browser,
                              TabStripModel* tab_strip_model,
                              int selected_index,
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

#if BUILDFLAG(ENABLE_CONTAINERS)
  if (base::FeatureList::IsEnabled(containers::features::kContainers)) {
    BuildItemForContainers(*browser->profile()->GetPrefs(), tab_strip_model,
                           containers_menu_model_delegate_.get(), indices);
  }
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

  if (base::FeatureList::IsEnabled(tabs::kBraveRenamingTabs)) {
    BuildItemForCustomization(tab_strip_model, selected_index);
  }

  // Replace SplitTabMenuModel with BraveSplitTabMenuModel.
  if (arrange_split_view_submenu_) {
    auto arrange_submenu_index =
        GetIndexOfCommandId(TabStripModel::CommandArrangeSplit);
    CHECK(arrange_submenu_index);
    RemoveItemAt(*arrange_submenu_index);
    arrange_split_view_submenu_ = std::make_unique<BraveSplitTabMenuModel>(
        tab_strip_model, SplitTabMenuModel::MenuSource::kTabContextMenu,
        selected_index);
    InsertSubMenuWithStringIdAt(
        *arrange_submenu_index, TabStripModel::CommandArrangeSplit,
        IDS_TAB_CXMENU_ARRANGE_SPLIT, arrange_split_view_submenu_.get());
    SetIcon(*arrange_submenu_index,
            ui::ImageModel::FromVectorIcon(kSplitSceneIcon, ui::kColorMenuIcon,
                                           16));
    SetElementIdentifierAt(*arrange_submenu_index, kArrangeSplitTabsMenuItem);
  }
}

#if BUILDFLAG(ENABLE_CONTAINERS)
void BraveTabMenuModel::BuildItemForContainers(
    const PrefService& prefs,
    TabStripModel* tab_strip_model,
    containers::ContainersMenuModel::Delegate& containers_delegate,
    const std::vector<int>& indices) {
  // There are multiple command ids that could be used to find the right
  // insertion point for the containers submenu. The command ids could be absent
  // depending on the tab state. So we check for multiple commands.
  std::optional<size_t> index;
  for (const auto& command_id : {
           TabStripModel::CommandMoveTabsToNewWindow,
           TabStripModel::CommandMoveToExistingWindow,
       }) {
    if (auto cmd_index = GetIndexOfCommandId(command_id)) {
      index = *cmd_index;
      break;
    }
  }
  CHECK(index.has_value());
  index = *index + 1;

  containers_submenu_ = std::make_unique<containers::ContainersMenuModel>(
      containers_delegate, prefs);
  InsertSubMenuWithStringIdAt(*index, CommandOpenInContainer,
                              IDS_CXMENU_OPEN_IN_CONTAINER,
                              containers_submenu_.get());
}
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

void BraveTabMenuModel::BuildItemForCustomization(
    TabStripModel* tab_strip_model,
    int tab_index) {
  if (tab_strip_model->IsTabPinned(tab_index)) {
    // In case of pinned tabs, we don't show titles at all, so we don't need to
    // show the rename option.
    return;
  }

  const auto index = *GetIndexOfCommandId(TabStripModel::CommandReload) + 1;
  InsertItemWithStringIdAt(index, CommandRenameTab, IDS_TAB_CXMENU_RENAME_TAB);
}
