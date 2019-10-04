/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/chromium_src/chrome/browser/chrome_browser_main_android.h"

#include "brave/browser/brave_browser_main_parts.h"
#include "chrome/browser/chrome_browser_main.h"

#define ChromeBrowserMainParts BraveBrowserMainParts
#include "../../../../chrome/browser/chrome_browser_main_android.cc"  // NOLINT
#undef ChromeBrowserMainParts
