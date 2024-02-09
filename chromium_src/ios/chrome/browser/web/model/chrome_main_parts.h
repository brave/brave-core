/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_WEB_MODEL_CHROME_MAIN_PARTS_H_
#define BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_WEB_MODEL_CHROME_MAIN_PARTS_H_

#include "ios/web/public/init/web_main_parts.h"

// Change `PreCreateMainMessageLoop` to `protected` so we can override the
// IE: Change all the ChromeMainParts functions to `protected` instead of
// `private.
#define PreCreateMainMessageLoop           \
  PreCreateMainMessageLoop_ChromiumImpl(); \
                                           \
 protected:                                \
  void PreCreateMainMessageLoop

#include "src/ios/chrome/browser/web/model/chrome_main_parts.h"  // IWYU pragma: export
#undef PreCreateMainMessageLoop

#endif  // BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_WEB_MODEL_CHROME_MAIN_PARTS_H_
