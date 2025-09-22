/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/policy/core/common/configuration_policy_provider.h"

namespace brave_policy {
std::unique_ptr<policy::ConfigurationPolicyProvider>
CreateBraveProfilePolicyProvider();
}  // namespace brave_policy

// Create and Init the Brave Profile Policy Provider which is used
// for Brave Origin and other Brave specific policies.
// We do not need to define a patch for Shutdown since anything in
// wrapped_policy_providers_ will automatically call Shutdown.
#define BRAVE_PROFILE_POLICY_CONNECTOR_INIT                         \
  auto provider = brave_policy::CreateBraveProfilePolicyProvider(); \
  brave_profile_policy_provider_ = provider.get();                  \
  policy_providers_.push_back(provider.get());                      \
  provider->Init(schema_registry);                                  \
  wrapped_policy_providers_.push_back(std::move(provider));

#include <chrome/browser/policy/profile_policy_connector.cc>  // IWYU pragma: export

namespace policy {
raw_ptr<policy::ConfigurationPolicyProvider>
ProfilePolicyConnector::GetBraveProfilePolicyProvider() {
  return brave_profile_policy_provider_;
}
}  // namespace policy

#undef BRAVE_PROFILE_POLICY_CONNECTOR_INIT
