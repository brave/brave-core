/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/supports_user_data.h"

// Replace the `WKWebViewConfigurationProvider` constructor call with the Brave
// subclass inside of `WKWebViewConfigurationProvider::FromBrowserState`
#define SetUserData(key, ...)                                                \
  SetUserData(key, base::WrapUnique(new BraveWKWebViewConfigurationProvider( \
                       browser_state)));
#include "src/ios/web/web_state/ui/wk_web_view_configuration_provider.mm"
#undef SetUserData

namespace web {

void BraveWKWebViewConfigurationProvider::ResetWithWebViewConfiguration(
    WKWebViewConfiguration* configuration) {
  WKWebViewConfigurationProvider::ResetWithWebViewConfiguration(configuration);

  // Adjusts the underlying WKWebViewConfiguration for settings we don't want
  // to inherit from Chromium

  // Restore WKWebView long press
  @try {
    [configuration_ setValue:@YES forKey:@"longPressActionsEnabled"];
  } @catch (NSException* exception) {
    NOTREACHED() << "Error setting value for longPressActionsEnabled";
  }

  // Restore Apple's safe browsing implementation
  [[configuration_ preferences] setFraudulentWebsiteWarningEnabled:YES];

  // Reset fullscreen to default as it wasn't set in Brave
  [[configuration_ preferences] setElementFullscreenEnabled:NO];
}

}  // namespace web
