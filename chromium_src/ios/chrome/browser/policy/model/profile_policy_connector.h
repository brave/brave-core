// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_POLICY_MODEL_PROFILE_POLICY_CONNECTOR_H_
#define BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_POLICY_MODEL_PROFILE_POLICY_CONNECTOR_H_

// Don't apply the renames to these files
#include "components/policy/core/common/policy_service_impl.h"

#define UseLocalTestPolicyProvider \
  UseLocalTestPolicyProvider();    \
  raw_ptr<policy::ConfigurationPolicyProvider> GetBraveProfilePolicyProvider

// Add in brave_profile_policy_provider_ so we can pass it the profile ID
#define ProfilePolicyConnectorMock                     \
  ProfilePolicyConnectorMock;                          \
  std::unique_ptr<policy::ConfigurationPolicyProvider> \
      brave_profile_policy_provider_

#include <ios/chrome/browser/policy/model/profile_policy_connector.h>  // IWYU pragma: export

#undef ProfilePolicyConnectorMock
#undef UseLocalTestPolicyProvider

#endif  // BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_POLICY_MODEL_PROFILE_POLICY_CONNECTOR_H_
