/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ssl/https_upgrades_interceptor.h"

// Prevent double-defining macro
#include "chrome/browser/renderer_host/chrome_navigation_ui_data.h"

// Suppress HTTPS upgrade if we're loading a .onion page in the Tor
// window, since we treat .onion domains as secure.
#define IsLocalhost(URL) IsLocalhostOrOnion(URL)

#include <chrome/browser/ssl/https_upgrades_interceptor.cc>

#undef IsLocalhost
