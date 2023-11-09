/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_content_browser_client.h"

#define ChromeContentBrowserClient BraveContentBrowserClient
#include "src/chrome/browser/ui/extensions/hosted_app_browsertest.cc"
#undef ChromeContentBrowserClient
