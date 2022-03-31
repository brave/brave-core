/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_APP_MENU_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_APP_MENU_H_

#include "chrome/browser/ui/views/toolbar/app_menu.h"

class BraveAppMenu : public AppMenu {
 public:
  using AppMenu::AppMenu;
  ~BraveAppMenu() override;

  BraveAppMenu(const BraveAppMenu&) = delete;
  BraveAppMenu& operator=(const BraveAppMenu&) = delete;

 private:
  // AppMenu overrides:
  views::MenuItemView* AddMenuItem(views::MenuItemView* parent,
                                   int menu_index,
                                   ui::MenuModel* model,
                                   int model_index,
                                   ui::MenuModel::ItemType menu_type) override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_APP_MENU_H_
