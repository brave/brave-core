/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/policy/chrome_browser_policy_connector.h"

#include "brave/components/brave_policy/brave_browser_policy_provider-forward.inc"

// Rename CreatePolicyProviders to CreatePolicyProviders_ChromiumImpl
#define CreatePolicyProviders CreatePolicyProviders_ChromiumImpl

#include <chrome/browser/policy/chrome_browser_policy_connector.cc>  // IWYU pragma: export

#undef CreatePolicyProviders

// And define the new one
namespace policy {

std::vector<std::unique_ptr<policy::ConfigurationPolicyProvider>>
ChromeBrowserPolicyConnector::CreatePolicyProviders() {
  auto providers =
      ChromeBrowserPolicyConnector::CreatePolicyProviders_ChromiumImpl();
  // Add browser policy provider for browser-level (local state) policies
  auto brave_browser_provider =
      ::brave_policy::CreateBraveBrowserPolicyProvider();
  // providers takes ownership of brave_browser_provider
  providers.push_back(std::move(brave_browser_provider));
  return providers;
}

}  // namespace policy
