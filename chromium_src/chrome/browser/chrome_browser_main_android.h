/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_CHROME_BROWSER_MAIN_ANDROID_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_CHROME_BROWSER_MAIN_ANDROID_H_

#include "brave/browser/brave_browser_main_parts.h"
#include "chrome/browser/chrome_browser_main.h"

#define ChromeBrowserMainParts BraveBrowserMainParts
#include "../../../../chrome/browser/chrome_browser_main_android.h"  // NOLINT
#undef ChromeBrowserMainParts

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_CHROME_BROWSER_MAIN_ANDROID_H_
