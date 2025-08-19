/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_POLICY_CORE_BROWSER_BROWSER_POLICY_CONNECTOR_BASE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_POLICY_CORE_BROWSER_BROWSER_POLICY_CONNECTOR_BASE_H_

// Insert a declaration for CreatePolicyProviders_ChromiumImpl because
// an override in
// chromium_src/chrome/browser/policy/chrome_browser_policy_connector.cc can't
// avoid calling a base class's CreatePolicyProviders_ChromiumImpl
#define HasPolicyService                                    \
  HasPolicyService();                                       \
  std::vector<std::unique_ptr<ConfigurationPolicyProvider>> \
      CreatePolicyProviders_ChromiumImpl

#include <components/policy/core/browser/browser_policy_connector_base.h>  // IWYU pragma: export

#undef HasPolicyService

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_POLICY_CORE_BROWSER_BROWSER_POLICY_CONNECTOR_BASE_H_
