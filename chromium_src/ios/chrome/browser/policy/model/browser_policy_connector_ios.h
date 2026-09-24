// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_POLICY_MODEL_BROWSER_POLICY_CONNECTOR_IOS_H_
#define BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_POLICY_MODEL_BROWSER_POLICY_CONNECTOR_IOS_H_

#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/policy_service.h"

// Rename CreatePolicyProviders to CreatePolicyProviders_ChromiumImpl
// and declare a new one
#define CreatePolicyProviders                                       \
  CreatePolicyProviders_ChromiumImpl();                             \
  std::vector<std::unique_ptr<policy::ConfigurationPolicyProvider>> \
      CreatePolicyProviders

#include <ios/chrome/browser/policy/model/browser_policy_connector_ios.h>  // IWYU pragma: export

#undef CreatePolicyProviders

#endif  // BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_POLICY_MODEL_BROWSER_POLICY_CONNECTOR_IOS_H_
