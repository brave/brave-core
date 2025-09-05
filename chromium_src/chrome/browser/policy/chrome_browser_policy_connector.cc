/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/policy/chrome_browser_policy_connector.h"

// Rename CreatePolicyProviders to CreatePolicyProviders_ChromiumImpl
#define CreatePolicyProviders CreatePolicyProviders_ChromiumImpl
// Just after InitInternal, Initialize brave_browser_provider_
#define InitInternal(a, b) \
  InitInternal(a, b);      \
  brave_browser_provider_->Initialize(local_state, GetSchemaRegistry());

#include <chrome/browser/policy/chrome_browser_policy_connector.cc>  // IWYU pragma: export

#undef InitInternal
#undef CreatePolicyProviders

// And define the new one
namespace policy {

std::vector<std::unique_ptr<policy::ConfigurationPolicyProvider>>
ChromeBrowserPolicyConnector::CreatePolicyProviders() {
  auto providers =
      ChromeBrowserPolicyConnector::CreatePolicyProviders_ChromiumImpl();

  // Add browser policy provider for browser-level (local state) policies
  auto brave_browser_provider =
      std::make_unique<brave_policy::BraveBrowserPolicyProvider>();
  brave_browser_provider_ = brave_browser_provider.get();
  providers.push_back(std::move(brave_browser_provider));
  return providers;
}

}  // namespace policy
