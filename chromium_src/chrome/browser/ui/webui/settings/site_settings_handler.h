/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SETTINGS_SITE_SETTINGS_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SETTINGS_SITE_SETTINGS_HANDLER_H_

#define SiteSettingsHandlerBaseTest \
  SiteSettingsHandlerBaseTest;      \
  friend class BraveSiteSettingsHandler
#define RemoveNonModelData virtual RemoveNonModelData

#include "src/chrome/browser/ui/webui/settings/site_settings_handler.h"  // IWYU pragma: export

#undef SiteSettingsHandlerBaseTest
#undef RemoveNonModelData

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SETTINGS_SITE_SETTINGS_HANDLER_H_
