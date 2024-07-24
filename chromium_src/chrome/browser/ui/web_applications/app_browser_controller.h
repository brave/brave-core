// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEB_APPLICATIONS_APP_BROWSER_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEB_APPLICATIONS_APP_BROWSER_CONTROLLER_H_

#define FormatUrlOrigin(...)                 \
  FormatUrlOrigin_ChromiumImpl(__VA_ARGS__); \
  static std::u16string FormatUrlOrigin(__VA_ARGS__)

#include "src/chrome/browser/ui/web_applications/app_browser_controller.h"  // IWYU pragma: export
#undef FormatUrlOrigin

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEB_APPLICATIONS_APP_BROWSER_CONTROLLER_H_
