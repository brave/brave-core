/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TOOLBAR_BRAVE_APP_MENU_MODEL_H_
#define BRAVE_BROWSER_UI_TOOLBAR_BRAVE_APP_MENU_MODEL_H_

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/gtest_prod_util.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "chrome/browser/ui/toolbar/app_menu_model.h"
#include "ui/menus/simple_menu_model.h"

#if defined(TOOLKIT_VIEWS)
#include "brave/components/sidebar/browser/sidebar_service.h"
#endif  // defined(TOOLKIT_VIEWS)

namespace ui {
class ButtonMenuItemModel;
}

class BraveAppMenuModel : public AppMenuModel {
 public:
  BraveAppMenuModel(ui::AcceleratorProvider* provider,
                    Browser* browser,
                    AppMenuIconController* app_menu_icon_controller = nullptr,
                    AlertMenuItem alert_item = AlertMenuItem::kNone);
  ~BraveAppMenuModel() override;

  BraveAppMenuModel(const BraveAppMenuModel&) = delete;
  BraveAppMenuModel& operator=(const BraveAppMenuModel&) = delete;

#if defined(TOOLKIT_VIEWS)
  static sidebar::SidebarService::ShowSidebarOption
  ConvertIDCToSidebarShowOptions(int id);
#endif  // defined(TOOLKIT_VIEWS)

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveAppMenuModelBrowserTest, BraveIpfsMenuTest);
  friend class BraveAppMenuModelBrowserTest;

  // AppMenuModel overrides:
  void Build() override;
  void ExecuteCommand(int id, int event_flags) override;
  bool IsCommandIdEnabled(int id) const override;

  std::optional<size_t> GetProperItemIndex(std::vector<int> commands_to_check,
                                           bool insert_next) const;

  void BuildTabsAndWindowsSection();

  // Leo(not yet added), Wallet, VPN and IPFS.
  void BuildBraveProductsSection();
  size_t GetNextIndexOfBraveProductsSection() const;

  // History, bookmarks, downloads and extensions.
  void BuildBrowserSection();

  // Insert profile, sidebar, sync and cast entries into existing more tools
  // sub menu.
  void BuildMoreToolsSubMenu();

  // About brave, help center and report broken site items.
  void BuildHelpSubMenu();

  // We relocate some upstream items. Need to remove them before adding them
  // to another location.
  void RemoveUpstreamMenus();

#if defined(TOOLKIT_VIEWS)
  std::unique_ptr<ui::ButtonMenuItemModel> sidebar_show_option_model_;
#endif  // defined(TOOLKIT_VIEWS)
};

#endif  // BRAVE_BROWSER_UI_TOOLBAR_BRAVE_APP_MENU_MODEL_H_
