/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/extension_util.h"

#include "brave/browser/extensions/updater/brave_update_client_config.h"
#include "chrome/browser/extensions/updater/chrome_update_client_config.h"

#define ChromeUpdateClientConfig BraveUpdateClientConfig
#include "src/chrome/browser/extensions/extension_util.cc"
#undef ChromeUpdateClientConfig
