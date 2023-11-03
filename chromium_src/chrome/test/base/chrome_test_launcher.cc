/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/test/base/chrome_test_launcher.h"

#include "brave/app/brave_main_delegate.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/utility/brave_content_utility_client.h"

#define ChromeContentUtilityClient BraveContentUtilityClient
#define ChromeContentBrowserClient BraveContentBrowserClient
#define ChromeMainDelegate BraveMainDelegate
#include "src/chrome/test/base/chrome_test_launcher.cc"
#undef ChromeMainDelegate
#undef ChromeContentBrowserClient
#undef ChromeContentUtilityClient
