/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTENSIONS_EXTENSION_UTIL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTENSIONS_EXTENSION_UTIL_H_

#define SetIsIncognitoEnabled                                      \
  SetIsTorEnabled(const std::string& extension_id,                 \
                  content::BrowserContext* context, bool enabled); \
  void SetIsIncognitoEnabled

#include "../../../../../chrome/browser/extensions/extension_util.h"
#undef SetIsIncognitoEnabled

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTENSIONS_EXTENSION_UTIL_H_
