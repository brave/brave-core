// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/web_view/brave_web_view_configuration_provider.h"

#include "base/memory/ptr_util.h"
#include "brave/ios/browser/api/web_view/brave_web_view_configuration.h"
#include "ios/web/public/browser_state.h"
#include "ios/web_view/internal/cwv_web_view_configuration_internal.h"
#include "ios/web_view/public/cwv_web_view_configuration.h"

namespace {

// A key used to associate a BraveWebViewConfigurationProvider with a
// BrowserState.
constexpr char kCWVWebViewConfigProviderKeyName[] =
    "brave_web_view_configuration_provider";

}  // namespace

// static
BraveWebViewConfigurationProvider&
BraveWebViewConfigurationProvider::FromBrowserState(
    web::BrowserState* browser_state) {
  DCHECK(browser_state);
  if (!browser_state->GetUserData(kCWVWebViewConfigProviderKeyName)) {
    browser_state->SetUserData(
        kCWVWebViewConfigProviderKeyName,
        base::WrapUnique(new BraveWebViewConfigurationProvider(browser_state)));
  }
  return *(static_cast<BraveWebViewConfigurationProvider*>(
      browser_state->GetUserData(kCWVWebViewConfigProviderKeyName)));
}

BraveWebViewConfiguration*
BraveWebViewConfigurationProvider::GetConfiguration() {
  if (!configuration_) {
    configuration_ =
        [[BraveWebViewConfiguration alloc] initWithBrowserState:browser_state_];
  }
  return configuration_;
}

void BraveWebViewConfigurationProvider::ResetConfiguration() {
  [configuration_ shutDown];
  configuration_ = nil;
}

BraveWebViewConfigurationProvider::BraveWebViewConfigurationProvider(
    web::BrowserState* browser_state)
    : browser_state_(browser_state) {}

BraveWebViewConfigurationProvider::~BraveWebViewConfigurationProvider() {
  [configuration_ shutDown];
}
