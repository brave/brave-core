/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/web/web_state/ui/wk_web_view_configuration_provider.h"

#include "base/notreached.h"
#include "base/supports_user_data.h"
#include "ios/web/public/web_client.h"

// Replace the `WKWebViewConfigurationProvider` constructor call with the Brave
// subclass inside of `WKWebViewConfigurationProvider::FromBrowserState`
#define SetUserData(key, ...)                                                \
  SetUserData(key, base::WrapUnique(new BraveWKWebViewConfigurationProvider( \
                       browser_state)));
#include <ios/web/web_state/ui/wk_web_view_configuration_provider.mm>
#undef SetUserData

namespace web {

void BraveWKWebViewConfigurationProvider::ResetWithWebViewConfiguration(
    WKWebViewConfiguration* configuration) {
  if (configuration != nil) {
    // We need to ensure that each tab has isolated WKUserContentController &
    // WKPreferences, because as of now we specifically adjust these values per
    // web view created rather than when the configuration is created.
    //
    // This must happen prior to WKWebView's creation.
    configuration.userContentController =
        [[WKUserContentController alloc] init];
    configuration.preferences = [configuration.preferences copy];
  }

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

void BraveWKWebViewConfigurationProvider::UpdateScripts() {
  WKWebViewConfigurationProvider::UpdateScripts();
  GetWebClient()->UpdateScripts();
}

}  // namespace web
