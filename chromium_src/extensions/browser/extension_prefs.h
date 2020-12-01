/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_EXTENSION_PREFS_H_
#define BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_EXTENSION_PREFS_H_

#define IsIncognitoEnabled                             \
  IsTorEnabled(const std::string& extension_id) const; \
  bool IsIncognitoEnabled

#define SetIsIncognitoEnabled                                      \
  SetIsTorEnabled(const std::string& extension_id, bool enabled); \
  void SetIsIncognitoEnabled

#include "../../../../extensions/browser/extension_prefs.h"

#undef IsIncognitoEnabled
#undef SetIsIncognitoEnabled

#endif  // BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_EXTENSION_PREFS_H_
