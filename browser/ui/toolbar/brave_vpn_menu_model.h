/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TOOLBAR_BRAVE_VPN_MENU_MODEL_H_
#define BRAVE_BROWSER_UI_TOOLBAR_BRAVE_VPN_MENU_MODEL_H_

#include "ui/base/models/simple_menu_model.h"

class Browser;

class BraveVPNMenuModel : public ui::SimpleMenuModel,
                          public ui::SimpleMenuModel::Delegate {
 public:
  explicit BraveVPNMenuModel(Browser* browser);
  ~BraveVPNMenuModel() override;

  BraveVPNMenuModel(const BraveVPNMenuModel&) = delete;
  BraveVPNMenuModel& operator=(const BraveVPNMenuModel&) = delete;

 private:
  // ui::SimpleMenuModel::Delegate override:
  void ExecuteCommand(int command_id, int event_flags) override;

  void Build();
  bool IsBraveVPNButtonVisible() const;

  Browser* browser_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_TOOLBAR_BRAVE_VPN_MENU_MODEL_H_
