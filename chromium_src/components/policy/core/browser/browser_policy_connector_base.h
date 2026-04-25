/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_POLICY_CORE_BROWSER_BROWSER_POLICY_CONNECTOR_BASE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_POLICY_CORE_BROWSER_BROWSER_POLICY_CONNECTOR_BASE_H_

// The reason for this override is to add CreatePolicyProviders_ChromiumImpl in
// BrowserPolicyConnector.
// We need that function because the base class's CreatePolicyProviders gets
// called from another override we did in
// chromium_src/chrome/browser/policy/chrome_browser_policy_connector.cc
// which renames CreatePolicyProviders to CreatePolicyProviders_ChromiumImpl,
// but that also renames the call to its base class's
// BrowserPolicyConnector::CreatePolicyProviders inside that same
// chrome/browser/policy/chrome_browser_policy_connector.cc file.
// eg.:
//
// std::vector<std::unique_ptr<policy::ConfigurationPolicyProvider>>
// ChromeBrowserPolicyConnector::CreatePolicyProviders() {
//   auto providers = BrowserPolicyConnector::CreatePolicyProviders(); <-----
//
// Since you can't just rename only the first instance, the base class also
// needs an override.
#define CreatePolicyProviders                               \
  CreatePolicyProviders();                                  \
  std::vector<std::unique_ptr<ConfigurationPolicyProvider>> \
      CreatePolicyProviders_ChromiumImpl

#include <components/policy/core/browser/browser_policy_connector_base.h>  // IWYU pragma: export

#undef CreatePolicyProviders

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_POLICY_CORE_BROWSER_BROWSER_POLICY_CONNECTOR_BASE_H_
