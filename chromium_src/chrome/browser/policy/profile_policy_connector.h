/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_POLICY_PROFILE_POLICY_CONNECTOR_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_POLICY_PROFILE_POLICY_CONNECTOR_H_

// Don't apply the renames to these files
#include "components/policy/core/common/policy_service_impl.h"

#define UseLocalTestPolicyProvider \
  UseLocalTestPolicyProvider();    \
  raw_ptr<policy::ConfigurationPolicyProvider> GetBraveProfilePolicyProvider

// Add in brave_profile_policy_provider_ so we can pass it the profile ID
#define local_test_policy_provider_      \
  local_test_policy_provider_ = nullptr; \
  raw_ptr<policy::ConfigurationPolicyProvider> brave_profile_policy_provider_

#include <chrome/browser/policy/profile_policy_connector.h>  // IWYU pragma: export

#undef UseLocalTestPolicyProvider
#undef local_test_policy_provider_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_POLICY_PROFILE_POLICY_CONNECTOR_H_
