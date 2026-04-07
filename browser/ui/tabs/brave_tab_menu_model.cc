/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_menu_model.h"

#include <algorithm>
#include <vector>

#include "base/check.h"
#include "base/feature_list.h"
#include "brave/browser/containers/containers_service_factory.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/brave_split_tab_menu_model.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_menu_model_delegate.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/sessions/core/tab_restore_service.h"
#include "components/tabs/public/tab_interface.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/menus/simple_menu_model.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/browser/ui/tabs/containers_tab_menu_model_delegate.h"
#include "brave/components/containers/core/common/features.h"
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

BraveTabMenuModel::BraveTabMenuModel(
    ui::SimpleMenuModel::Delegate* delegate,
    TabMenuModelDelegate* tab_menu_model_delegate,
    TabStripModel* tab_strip_model,
    int index)
    : TabMenuModel(delegate, tab_menu_model_delegate, tab_strip_model, index) {
  auto* tab = tab_strip_model->GetTabAtIndex(index);
  CHECK(tab);
  auto* browser_window = tab->GetBrowserWindowInterface();
  CHECK(browser_window);

  restore_service_ =
      TabRestoreServiceFactory::GetForProfile(browser_window->GetProfile());

  auto* model = static_cast<BraveTabStripModel*>(tab_strip_model);
  auto indices = model->GetTabIndicesForCommandAt(index);
  all_muted_ = model->GetAllTabsMuted(indices);
  Build(browser_window, tab_strip_model, index, indices);
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
  if (!tab_menu_model_delegate_->ShouldShowBraveVerticalTab()) {
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

void BraveTabMenuModel::Build(BrowserWindowInterface* browser_window,
                              TabStripModel* tab_strip_model,
                              int selected_index,
                              const std::vector<int>& indices) {
  auto selected_tab_count = indices.size();

  AddSeparator(ui::NORMAL_SEPARATOR);
  auto mute_site_index =
      GetIndexOfCommandId(TabStripModel::CommandToggleSiteMuted);

  auto toggle_tab_mute_label = l10n_util::GetPluralStringFUTF16(
      all_muted_ ? IDS_TAB_CXMENU_SOUND_UNMUTE_TAB
                 : IDS_TAB_CXMENU_SOUND_MUTE_TAB,
      selected_tab_count);
  InsertItemAt(mute_site_index.value_or(GetItemCount()),
               TabStripModel::CommandToggleTabMuted, toggle_tab_mute_label);

  AddItemWithStringId(TabStripModel::CommandRestoreTab,
                      GetRestoreTabCommandStringId());
  AddItemWithStringId(TabStripModel::CommandBookmarkAllTabs,
                      IDS_TAB_CXMENU_BOOKMARK_ALL_TABS);

  // TODO(https://github.com/brave/brave-browser/issues/54241): Update
  // brave::CanBringAllTabs() to accept BrowserWindowInterface*.
  if (brave::CanBringAllTabs(browser_window->GetBrowserForMigrationOnly())) {
    AddItemWithStringId(TabStripModel::CommandBringAllTabsToThisWindow,
                        IDS_TAB_CXMENU_BRING_ALL_TABS_TO_THIS_WINDOW);
  }

  AddSeparator(ui::NORMAL_SEPARATOR);

  if (tabs::utils::SupportsBraveVerticalTabs(browser_window)) {
    AddCheckItemWithStringId(TabStripModel::CommandShowVerticalTabs,
                             IDS_TAB_CXMENU_SHOW_VERTICAL_TABS);
  }

  auto close_other_tabs_index =
      GetIndexOfCommandId(TabStripModel::CommandCloseOtherTabs);
  InsertItemWithStringIdAt(close_other_tabs_index.value_or(GetItemCount()),
                           TabStripModel::CommandCloseDuplicateTabs,
                           IDS_TAB_CXMENU_CLOSE_DUPLICATE_TABS);

#if BUILDFLAG(ENABLE_CONTAINERS)
  if (base::FeatureList::IsEnabled(containers::features::kContainers)) {
    BuildItemForContainers(browser_window, tab_strip_model, indices);
  }
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

  // Reorder the split tab entry to the last position of the first section.
  // For CommandArrangeSplit, also replaces upstream's SplitTabMenuModel with
  // BraveSplitTabMenuModel.
  BuildSplitTabEntry(tab_strip_model, selected_index);
}

void BraveTabMenuModel::BuildSplitTabEntry(TabStripModel* tab_strip_model,
                                           int selected_index) {
  // Find whichever split-related command is present in the menu.
  std::optional<int> split_cmd;
  size_t split_index = 0;
  for (int cmd :
       {TabStripModel::CommandArrangeSplit, TabStripModel::CommandAddToSplit,
        TabStripModel::CommandSwapWithActiveSplit}) {
    if (auto index = GetIndexOfCommandId(cmd)) {
      split_cmd = cmd;
      split_index = *index;
      break;
    }
  }
  if (!split_cmd) {
    return;
  }

  // Capture item properties before removal so we can re-insert at the target.
  const auto label = GetLabelAt(split_index);
  const auto icon = GetIconAt(split_index);
  const bool is_submenu = GetTypeAt(split_index) == ui::MenuModel::TYPE_SUBMENU;
  const bool enabled = IsEnabledAt(split_index);
  const auto element_id = GetElementIdentifierAt(split_index);
  ui::MenuModel* submenu =
      is_submenu ? GetSubmenuModelAt(split_index) : nullptr;

  RemoveItemAt(split_index);
  // Find the first separator which marks the end of the first section.
  // If no separator exists, append at the end.
  size_t insert_index = GetItemCount();
  for (size_t i = 0; i < GetItemCount(); i++) {
    if (GetTypeAt(i) == ui::MenuModel::TYPE_SEPARATOR) {
      insert_index = i;
      break;
    }
  }
  if (*split_cmd == TabStripModel::CommandArrangeSplit) {
    arrange_split_view_submenu_ = std::make_unique<BraveSplitTabMenuModel>(
        tab_strip_model, SplitTabMenuModel::MenuSource::kTabContextMenu,
        selected_index);
    InsertSubMenuWithStringIdAt(
        insert_index, TabStripModel::CommandArrangeSplit,
        IDS_TAB_CXMENU_ARRANGE_SPLIT, arrange_split_view_submenu_.get());
  } else {
    if (submenu) {
      InsertSubMenuAt(insert_index, *split_cmd, label, submenu);
    } else {
      InsertItemAt(insert_index, *split_cmd, label);
    }
  }
  SetEnabledAt(insert_index, enabled);
  SetIcon(insert_index, icon);
  if (element_id) {
    SetElementIdentifierAt(insert_index, element_id);
  }
}

#if BUILDFLAG(ENABLE_CONTAINERS)
void BraveTabMenuModel::BuildItemForContainers(
    BrowserWindowInterface* browser_window,
    TabStripModel* tab_strip_model,
    const std::vector<int>& selected_tab_indices) {
  auto* service =
      ContainersServiceFactory::GetForProfile(browser_window->GetProfile());
  if (!service) {
    return;
  }

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

  std::vector<tabs::TabHandle> selected_tab_handles;
  for (auto selected_tab_index : selected_tab_indices) {
    auto* tab = tab_strip_model->GetTabAtIndex(selected_tab_index);
    CHECK(tab);
    selected_tab_handles.push_back(tab->GetHandle());
  }

  containers_menu_delegate_ =
      std::make_unique<brave::ContainersTabMenuModelDelegate>(
          browser_window, selected_tab_handles);
  containers_submenu_ = std::make_unique<containers::ContainersMenuModel>(
      *containers_menu_delegate_, *service);
  InsertSubMenuWithStringIdAt(*index, TabStripModel::CommandOpenInContainer,
                              IDS_CXMENU_OPEN_IN_CONTAINER,
                              containers_submenu_.get());
}
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
