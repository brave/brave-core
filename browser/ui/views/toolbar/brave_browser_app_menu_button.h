/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_BROWSER_APP_MENU_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_BROWSER_APP_MENU_BUTTON_H_

#include <optional>

#include "base/gtest_prod_util.h"
#include "chrome/browser/ui/views/toolbar/browser_app_menu_button.h"

class BraveBrowserAppMenuButton : public BrowserAppMenuButton {
  METADATA_HEADER(BraveBrowserAppMenuButton, BrowserAppMenuButton)

 public:
  using BrowserAppMenuButton::BrowserAppMenuButton;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveAppMenuBrowserTest,
                           AppMenuButtonUpgradeAlertTest);

  std::optional<SkColor> GetColorForSeverity() const;

  // BrowserAppMenuButton overrides:
  bool ShouldPaintBorder() const override;
  bool ShouldBlendHighlightColor() const override;
  std::optional<SkColor> GetHighlightTextColor() const override;
  std::optional<SkColor> GetHighlightColor() const override;
  SkColor GetForegroundColor(ButtonState state) const override;
  void UpdateLayoutInsets() override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_BROWSER_APP_MENU_BUTTON_H_
