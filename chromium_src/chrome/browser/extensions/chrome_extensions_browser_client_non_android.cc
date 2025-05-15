/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_extensions_api_client.h"
#include "chrome/browser/extensions/api/chrome_extensions_api_client.h"
#include "chrome/browser/extensions/chrome_extensions_browser_client.h"

#define ChromeExtensionsAPIClient BraveExtensionsAPIClient
#include "src/chrome/browser/extensions/chrome_extensions_browser_client_non_android.cc"
#undef ChromeExtensionsAPIClient
