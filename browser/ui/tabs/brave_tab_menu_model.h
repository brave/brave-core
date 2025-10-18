/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_BRAVE_TAB_MENU_MODEL_H_
#define BRAVE_BROWSER_UI_TABS_BRAVE_TAB_MENU_MODEL_H_

#include <memory>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/app/brave_command_ids.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/ui/tabs/tab_menu_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "ui/menus/simple_menu_model.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/browser/ui/containers/containers_menu_model.h"
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

namespace sessions {
class TabRestoreService;
}  // namespace sessions

class Browser;

class BraveTabMenuModel : public TabMenuModel {
 public:
  enum BraveTabContextMenuCommand {
    CommandStart = TabStripModel::CommandLast,
    CommandRestoreTab,
    CommandBookmarkAllTabs,
    CommandShowVerticalTabs,
    CommandToggleTabMuted,
    CommandBringAllTabsToThisWindow,
    CommandCloseDuplicateTabs,
    CommandOpenInContainer,
    CommandRenameTab,
    CommandLast,
  };

  static_assert(CommandLast < IDC_OPEN_IN_CONTAINER_START,
                "Container's menu commands must be after "
                "BraveTabContextMenuCommand to avoid conflicts");

  BraveTabMenuModel(
      ui::SimpleMenuModel::Delegate* delegate,
      TabMenuModelDelegate* tab_menu_model_delegate,
      TabStripModel* tab_strip_model,
#if BUILDFLAG(ENABLE_CONTAINERS)
      containers::ContainersMenuModel::Delegate& containers_delegate,
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
      int index,
      bool is_vertical_tab);
  BraveTabMenuModel(const BraveTabMenuModel&) = delete;
  BraveTabMenuModel& operator=(const BraveTabMenuModel&) = delete;
  ~BraveTabMenuModel() override;

  bool all_muted() const { return all_muted_; }

  // TabMenuModel:
  std::u16string GetLabelAt(size_t index) const override;
  bool IsNewFeatureAt(size_t index) const override;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveTabContextMenuContentsTest,
                           SplitViewMenuCustomizationTest);

  void Build(Browser* browser,
             TabStripModel* tab_strip_model,
             int selected_index,
             const std::vector<int>& indices);
  void BuildItemsForSplitView(Browser* browser,
                              TabStripModel* tab_strip_model,
                              const std::vector<int>& indices);
  int GetRestoreTabCommandStringId() const;

#if BUILDFLAG(ENABLE_CONTAINERS)
  void BuildItemForContainers(
      const PrefService& prefs,
      TabStripModel* tab_strip_model,
      containers::ContainersMenuModel::Delegate& containers_delegate,
      const std::vector<int>& indices);
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

  // Build menu items for tab customization, such as renaming the tab.
  void BuildItemForCustomization(TabStripModel* tab_strip_model, int tab_index);

  ui::SimpleMenuModel* arrange_split_view_submenu_for_testing() const {
    return arrange_split_view_submenu_.get();
  }

  raw_ptr<sessions::TabRestoreService> restore_service_ = nullptr;
  bool all_muted_;

  bool is_vertical_tab_ = false;

#if BUILDFLAG(ENABLE_CONTAINERS)
  raw_ref<containers::ContainersMenuModel::Delegate>
      containers_menu_model_delegate_;
  std::unique_ptr<containers::ContainersMenuModel> containers_submenu_;
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
};

#endif  // BRAVE_BROWSER_UI_TABS_BRAVE_TAB_MENU_MODEL_H_
