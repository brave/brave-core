/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>

#include "base/notreached.h"

#define BRAVE_RESET_WITH_WEB_VIEW_CONFIGURATION \
  brave::ResetWithWebViewConfiguration(configuration_);

namespace brave {

// FIXME: Replace patch + override with WKWebViewConfigurationProviderObserver
void ResetWithWebViewConfiguration(WKWebViewConfiguration* configuration) {
  // Adjusts the underlying WKWebViewConfiguration for settings we don't want
  // to inherit from Chromium

  // Restore WKWebView long press
  @try {
    [configuration setValue:@YES forKey:@"longPressActionsEnabled"];
  } @catch (NSException* exception) {
    NOTREACHED_IN_MIGRATION()
        << "Error setting value for longPressActionsEnabled";
  }

  // Restore Apple's safe browsing implementation
  [[configuration preferences] setFraudulentWebsiteWarningEnabled:YES];

  // Reset fullscreen to default as it wasn't set in Brave
  [[configuration preferences] setElementFullscreenEnabled:NO];
}

}  // namespace brave

#include "src/ios/web/web_state/ui/wk_web_view_configuration_provider.mm"

#undef BRAVE_RESET_WITH_WEB_VIEW_CONFIGURATION
