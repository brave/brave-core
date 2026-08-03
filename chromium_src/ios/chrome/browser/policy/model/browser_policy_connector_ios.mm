// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/chrome/browser/policy/model/browser_policy_connector_ios.h"

#include "brave/components/brave_policy/brave_browser_policy_provider-forward.inc"

#define CreatePolicyProviders CreatePolicyProviders_ChromiumImpl

#include <ios/chrome/browser/policy/model/browser_policy_connector_ios.mm>

#undef CreatePolicyProviders

// And define the new one
std::vector<std::unique_ptr<policy::ConfigurationPolicyProvider>>
BrowserPolicyConnectorIOS::CreatePolicyProviders() {
  auto providers =
      BrowserPolicyConnectorIOS::CreatePolicyProviders_ChromiumImpl();
  // Add browser policy provider for browser-level (local state) policies
  auto brave_browser_provider =
      ::brave_policy::CreateBraveBrowserPolicyProvider();
  // providers takes ownership of brave_browser_provider
  providers.push_back(std::move(brave_browser_provider));
  return providers;
}
