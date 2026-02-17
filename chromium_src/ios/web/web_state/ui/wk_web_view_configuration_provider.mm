/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/web/web_state/ui/wk_web_view_configuration_provider.h"

#include "base/supports_user_data.h"
#include "ios/web/public/browser_state.h"

namespace brave {
web::WKWebViewConfigurationProvider* CreateBraveWKWebViewConfigurationProvider(
    web::BrowserState* browser_state);
}

// Replace the `WKWebViewConfigurationProvider` constructor call with the Brave
// subclass inside of `WKWebViewConfigurationProvider::FromBrowserState`
#define SetUserData(key, ...)                                                 \
  SetUserData(                                                                \
      key, base::WrapUnique(brave::CreateBraveWKWebViewConfigurationProvider( \
               browser_state)));
#include <ios/web/web_state/ui/wk_web_view_configuration_provider.mm>
#undef SetUserData
