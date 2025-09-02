/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_POLICY_CHROME_BROWSER_POLICY_CONNECTOR_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_POLICY_CHROME_BROWSER_POLICY_CONNECTOR_H_

#include "base/gtest_prod_util.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/policy_service.h"

// Rename CreatePolicyProviders to CreatePolicyProviders_ChromiumImpl
// and declare a new one
#define CreatePolicyProviders                                       \
  CreatePolicyProviders_ChromiumImpl();                             \
  std::vector<std::unique_ptr<policy::ConfigurationPolicyProvider>> \
      CreatePolicyProviders

#include <chrome/browser/policy/chrome_browser_policy_connector.h>  // IWYU pragma: export

#undef CreatePolicyProviders

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_POLICY_CHROME_BROWSER_POLICY_CONNECTOR_H_
