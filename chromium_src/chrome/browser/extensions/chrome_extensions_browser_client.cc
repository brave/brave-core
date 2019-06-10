// Copyright (c) 2019 The Brave Authors. All rights reserved.

#include "chrome/browser/extensions/chrome_extensions_browser_client.h"
#include "brave/browser/extensions/updater/brave_update_client_config.h"
#include "chrome/browser/extensions/updater/chrome_update_client_config.h"

#define ChromeUpdateClientConfig BraveUpdateClientConfig
#include "../../../../../chrome/browser/extensions/chrome_extensions_browser_client.cc"  // NOLINT
#undef ChromeUpdateClientConfig
