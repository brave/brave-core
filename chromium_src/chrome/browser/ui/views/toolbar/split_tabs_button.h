/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_SPLIT_TABS_BUTTON_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_SPLIT_TABS_BUTTON_H_

#include <memory>

namespace ui {
class SimpleMenuModel;
}  // namespace ui

#define GetIconsForTesting(...)                       \
  GetIconsForTesting(__VA_ARGS__);                    \
  ui::SimpleMenuModel* split_tab_menu_for_testing() { \
    return split_tab_menu_.get();                     \
  }                                                   \
  void SetMenuModel(std::unique_ptr<ui::SimpleMenuModel> menu)

#include <chrome/browser/ui/views/toolbar/split_tabs_button.h>  // IWYU pragma: export

#undef GetIconsForTesting

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_SPLIT_TABS_BUTTON_H_
