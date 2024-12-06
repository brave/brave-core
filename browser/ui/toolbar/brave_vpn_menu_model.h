/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TOOLBAR_BRAVE_VPN_MENU_MODEL_H_
#define BRAVE_BROWSER_UI_TOOLBAR_BRAVE_VPN_MENU_MODEL_H_

#include <optional>

#include "base/memory/raw_ptr.h"
#include "ui/menus/simple_menu_model.h"

class Browser;
class PrefService;

class BraveVPNMenuModel : public ui::SimpleMenuModel,
                          public ui::SimpleMenuModel::Delegate {
 public:
  BraveVPNMenuModel(Browser* browser, PrefService* profile_prefs);
  ~BraveVPNMenuModel() override;

  BraveVPNMenuModel(const BraveVPNMenuModel&) = delete;
  BraveVPNMenuModel& operator=(const BraveVPNMenuModel&) = delete;
#if BUILDFLAG(IS_WIN)
  void SetTrayIconEnabledForTesting(bool value) {
    tray_icon_enabled_for_testing_ = value;
  }
#endif  // BUILDFLAG(IS_WIN)
 private:
  FRIEND_TEST_ALL_PREFIXES(BraveVPNMenuModelUnitTest, TrayIconEnabled);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNMenuModelUnitTest, TrayIconDisabled);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNMenuModelUnitTest, ToolbarVPNButton);

  // ui::SimpleMenuModel::Delegate override:
  void ExecuteCommand(int command_id, int event_flags) override;

  void Build();
  bool IsBraveVPNButtonVisible() const;
#if BUILDFLAG(IS_WIN)
  bool IsTrayIconEnabled() const;
#endif  // BUILDFLAG(IS_WIN)
  std::optional<bool> tray_icon_enabled_for_testing_;
  raw_ptr<PrefService> profile_prefs_ = nullptr;
  raw_ptr<Browser> browser_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_TOOLBAR_BRAVE_VPN_MENU_MODEL_H_
