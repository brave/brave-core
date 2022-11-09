/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_APP_MENU_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_APP_MENU_H_

#include <memory>

#include "chrome/browser/ui/views/toolbar/app_menu.h"

namespace misc_metrics {
class MenuMetrics;
}  // namespace misc_metrics

class BraveAppMenu : public AppMenu {
 public:
  using AppMenu::AppMenu;

  BraveAppMenu(Browser* browser, int run_types);
  ~BraveAppMenu() override;

  BraveAppMenu(const BraveAppMenu&) = delete;
  BraveAppMenu& operator=(const BraveAppMenu&) = delete;

  void ExecuteCommand(int command_id, int mouse_event_flags) override;

 private:
  // AppMenu overrides:
  views::MenuItemView* AddMenuItem(views::MenuItemView* parent,
                                   size_t menu_index,
                                   ui::MenuModel* model,
                                   size_t model_index,
                                   ui::MenuModel::ItemType menu_type) override;

  void RecordMenuUsage(int command_id);

  std::unique_ptr<misc_metrics::MenuMetrics> menu_metrics_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_APP_MENU_H_
