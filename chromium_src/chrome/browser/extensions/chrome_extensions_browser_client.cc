/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_component_extension_resource_manager.h"
#include "brave/browser/extensions/brave_process_manager_delegate.h"
#include "brave/browser/extensions/updater/brave_update_client_config.h"
#include "chrome/browser/extensions/updater/chrome_update_client_config.h"

#define ChromeComponentExtensionResourceManager BraveComponentExtensionResourceManager
#define ChromeProcessManagerDelegate BraveProcessManagerDelegate
#define ChromeUpdateClientConfig BraveUpdateClientConfig
#include "../../../../../chrome/browser/extensions/chrome_extensions_browser_client.cc"  // NOLINT
#undef ChromeComponentExtensionResourceManager
#undef ChromeProcessManagerDelegate
#undef ChromeUpdateClientConfig
