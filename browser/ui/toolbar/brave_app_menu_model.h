/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TOOLBAR_BRAVE_APP_MENU_MODEL_H_
#define BRAVE_BROWSER_UI_TOOLBAR_BRAVE_APP_MENU_MODEL_H_

#include "chrome/browser/ui/toolbar/app_menu_model.h"

class BraveAppMenuModel : public AppMenuModel {
 public:
  using AppMenuModel::AppMenuModel;
  ~BraveAppMenuModel() override;

  BraveAppMenuModel(const BraveAppMenuModel&) = delete;
  BraveAppMenuModel& operator=(const BraveAppMenuModel&) = delete;

 private:
  // AppMenuModel overrides:
  void Build() override;

  void InsertBraveMenuItems();
  int GetIndexOfBraveRewardsItem() const;
  int GetIndexOfBraveAdBlockItem() const;
  int GetIndexOfBraveSyncItem() const;
};

#endif  // BRAVE_BROWSER_UI_TOOLBAR_BRAVE_APP_MENU_MODEL_H_
