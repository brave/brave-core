/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SETTINGS_SETTINGS_DEFAULT_BROWSER_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SETTINGS_SETTINGS_DEFAULT_BROWSER_HANDLER_H_

#include "chrome/browser/shell_integration.h"

#define SetAsDefaultBrowser                \
  UnUsed() {}                              \
  friend class BraveDefaultBrowserHandler; \
  virtual void SetAsDefaultBrowser

#include "src/chrome/browser/ui/webui/settings/settings_default_browser_handler.h"
#undef SetAsDefaultBrowser

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SETTINGS_SETTINGS_DEFAULT_BROWSER_HANDLER_H_
