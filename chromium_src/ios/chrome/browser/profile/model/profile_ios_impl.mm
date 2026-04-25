// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/policy/core/common/configuration_policy_provider.h"
#include "ios/chrome/browser/policy/model/profile_policy_connector_factory.h"

namespace brave_policy {
class BraveProfilePolicyProvider;
void SetBraveProfilePolicyProviderProfileID(
    policy::ConfigurationPolicyProvider* provider,
    const base::FilePath& profile_path);
}  // namespace brave_policy

// Set the profile id after the profile policy connector has been created
// policy_connector_ is assigned to the result of BuildProfilePolicyConnector
#define BuildProfilePolicyConnector(...)                          \
  BuildProfilePolicyConnector(__VA_ARGS__);                       \
  if (policy_connector_->GetBraveProfilePolicyProvider()) {       \
    brave_policy::SetBraveProfilePolicyProviderProfileID(         \
        policy_connector_->GetBraveProfilePolicyProvider().get(), \
        GetStatePath());                                          \
  }

#include <ios/chrome/browser/profile/model/profile_ios_impl.mm>

#undef BuildProfilePolicyConnector
