/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTENSIONS_API_SETTINGS_PRIVATE_PREFS_UTIL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTENSIONS_API_SETTINGS_PRIVATE_PREFS_UTIL_H_

#define GetAllowlistedKeys \
  Dummy();                 \
  virtual const TypedPrefMap& GetAllowlistedKeys

#include "src/chrome/browser/extensions/api/settings_private/prefs_util.h"
#undef GetAllowlistedKeys

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTENSIONS_API_SETTINGS_PRIVATE_PREFS_UTIL_H_
