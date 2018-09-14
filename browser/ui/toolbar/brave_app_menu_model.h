/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BRWSER_UI_TOOLBAR_BRAVE_APP_MENU_MODEL_H_
#define BRAVE_BRWSER_UI_TOOLBAR_BRAVE_APP_MENU_MODEL_H_

#include "chrome/browser/ui/toolbar/app_menu_model.h"

class BraveAppMenuModel : public AppMenuModel {
 public:
  using AppMenuModel::AppMenuModel;
  BraveAppMenuModel(ui::AcceleratorProvider* provider, Browser* browser);
  ~BraveAppMenuModel() override;

 private:
  // AppMenuModel overrides:
  void Build() override;

  void InsertBraveMenuItems();

  Browser* const browser_;  // weak

  DISALLOW_COPY_AND_ASSIGN(BraveAppMenuModel);
};

#endif  // BRAVE_BRWSER_UI_TOOLBAR_BRAVE_APP_MENU_MODEL_H_
