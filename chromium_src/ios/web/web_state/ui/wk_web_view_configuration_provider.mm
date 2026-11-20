/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/web/web_state/ui/wk_web_view_configuration_provider.h"

#include <WebKit/WebKit.h>

#include "base/notreached.h"
#include "ios/web/public/web_client.h"

namespace web {

namespace {

// Adjusts `configuration` after `WKWebViewConfigurationProvider` resets it, for
// settings Brave doesn't inherit from Chromium.
void BraveDidResetConfiguration(BrowserState* browser_state,
                                WKWebViewConfiguration* configuration) {
  // Restore WKWebView long press
  @try {
    [configuration setValue:@YES forKey:@"longPressActionsEnabled"];
  } @catch (NSException* exception) {
    NOTREACHED() << "Error setting value for longPressActionsEnabled";
  }

  // Restore Apple's safe browsing implementation
  [[configuration preferences] setFraudulentWebsiteWarningEnabled:YES];

  // Reset fullscreen to default as it wasn't set in Brave
  [[configuration preferences] setElementFullscreenEnabled:NO];

  // Add Brave-specific adjustments to the WKWebViewConfiguration here

  configuration.dataDetectorTypes = WKDataDetectorTypePhoneNumber;

  // Explicitly pass in the private configuration so that it can be mutated
  // correctly prior to being used in WebState. This is a temporary measure
  // and can be removed once:
  // - The `internal` scheme is removed and interstitials are converted to WebUI
  //   https://github.com/brave/brave-browser/issues/53028
  // - We replace WebKit's built in safe browsing implementation with Chromiums
  //   https://github.com/brave/brave-browser/issues/53029
  // - We replace our custom HTTPS-only upgrade and can remove the assignment of
  //   `WKWebViewConfiguration.upgradeKnownHostsToHTTPS`
  //   https://github.com/brave/brave-browser/issues/53030
  GetWebClient()->DidResetConfiguration(browser_state, configuration);
}

}  // namespace

}  // namespace web

#include <ios/web/web_state/ui/wk_web_view_configuration_provider.mm>
