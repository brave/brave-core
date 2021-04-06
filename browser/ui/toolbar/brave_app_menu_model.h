/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TOOLBAR_BRAVE_APP_MENU_MODEL_H_
#define BRAVE_BROWSER_UI_TOOLBAR_BRAVE_APP_MENU_MODEL_H_

#include <memory>
#include <vector>

#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/sidebar/buildflags/buildflags.h"
#include "chrome/browser/ui/toolbar/app_menu_model.h"
#include "ui/base/models/simple_menu_model.h"

class BraveAppMenuModel : public AppMenuModel {
 public:
  BraveAppMenuModel(ui::AcceleratorProvider* provider,
                    Browser* browser,
                    AppMenuIconController* app_menu_icon_controller = nullptr);
  ~BraveAppMenuModel() override;

  BraveAppMenuModel(const BraveAppMenuModel&) = delete;
  BraveAppMenuModel& operator=(const BraveAppMenuModel&) = delete;

 private:
  // AppMenuModel overrides:
  void Build() override;

  void InsertBraveMenuItems();
  void InsertAlternateProfileItems();
  int GetIndexOfBraveRewardsItem() const;
  int GetIndexOfBraveAdBlockItem() const;
  int GetIndexOfBraveSyncItem() const;
#if BUILDFLAG(ENABLE_SIDEBAR)
  int GetIndexOfBraveSidebarItem() const;
#endif
#if BUILDFLAG(IPFS_ENABLED)
  ui::SimpleMenuModel ipfs_submenu_model_;
#endif
  std::vector<std::unique_ptr<ui::SimpleMenuModel>> sub_menus_;
};

#endif  // BRAVE_BROWSER_UI_TOOLBAR_BRAVE_APP_MENU_MODEL_H_
