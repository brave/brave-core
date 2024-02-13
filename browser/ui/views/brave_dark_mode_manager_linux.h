/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_DARK_MODE_MANAGER_LINUX_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_DARK_MODE_MANAGER_LINUX_H_

#include "chrome/browser/ui/views/dark_mode_manager_linux.h"

namespace ui {

class BraveDarkModeManagerLinux : public DarkModeManagerLinux {
 public:
  BraveDarkModeManagerLinux();
  ~BraveDarkModeManagerLinux() override;

 private:
  // DarkModeManagerLinux overrides:
  void SetColorScheme(bool prefer_dark_theme, bool from_toolkit_theme) override;
};

}  // namespace ui

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_DARK_MODE_MANAGER_LINUX_H_
