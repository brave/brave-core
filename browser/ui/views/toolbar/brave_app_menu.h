/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_APP_MENU_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_APP_MENU_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/misc_metrics/menu_metrics.h"
#include "chrome/browser/ui/views/toolbar/app_menu.h"

class BraveAppMenu : public AppMenu {
 public:
  BraveAppMenu(Browser* browser, ui::MenuModel* model, int run_types);
  ~BraveAppMenu() override;

  BraveAppMenu(const BraveAppMenu&) = delete;
  BraveAppMenu& operator=(const BraveAppMenu&) = delete;

  void RunMenu(views::MenuButtonController* host) override;

  void ExecuteCommand(int command_id, int mouse_event_flags) override;

  void OnMenuClosed(views::MenuItemView* menu) override;

 private:
  // AppMenu overrides:
  views::MenuItemView* AddMenuItem(views::MenuItemView* parent,
                                   size_t menu_index,
                                   ui::MenuModel* model,
                                   size_t model_index,
                                   ui::MenuModel::ItemType menu_type) override;

  void RecordMenuUsage(int command_id);

  base::raw_ptr<misc_metrics::MenuMetrics> menu_metrics_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_APP_MENU_H_
