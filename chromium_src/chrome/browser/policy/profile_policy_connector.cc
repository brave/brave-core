/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/profile_id.h"
#include "brave/components/brave_policy/brave_profile_policy_provider.h"
#include "components/policy/core/common/configuration_policy_provider.h"

namespace brave_policy {
std::unique_ptr<policy::ConfigurationPolicyProvider>
CreateBraveProfilePolicyProvider();
}  // namespace brave_policy

// Create and Init the Brave Profile Policy Provider which is used
// for Brave Origin and other Brave specific policies.
// We do not need to define a patch for Shutdown since anything in
// wrapped_policy_providers_ will automatically call Shutdown.
//
// If a caller stashed a profile path via
// BraveProfilePolicyProvider::SetPendingProfilePath() immediately before
// the policy connector is built, consume it and call SetProfileID BEFORE
// provider->Init(). That ordering matters: Init's Observe() call fires
// OnBravePoliciesReady synchronously (when BraveOriginPolicyManager is
// already initialized at browser-process startup), and since profile_id_
// is now set, OnBravePoliciesReady triggers RefreshPolicies → LoadPolicies
// → UpdatePolicy. The provider's bundle is therefore populated before
// PolicyServiceImpl's constructor performs its synchronous merge of all
// providers, so the resulting policy_bundle_ already carries our policies.
// Without this, PolicyServiceImpl would read an empty bundle from us and
// the later async merge (posted from OnUpdatePolicy) would have to race
// the consumers reading prefs from the resulting PrefService.
#define BRAVE_PROFILE_POLICY_CONNECTOR_INIT                                   \
  auto provider = brave_policy::CreateBraveProfilePolicyProvider();           \
  if (auto pending_path =                                                     \
          brave_policy::BraveProfilePolicyProvider::TakePendingProfilePath(); \
      !pending_path.empty()) {                                                \
    static_cast<brave_policy::BraveProfilePolicyProvider*>(provider.get())    \
        ->SetProfileID(brave_origin::GetProfileId(pending_path));             \
  }                                                                           \
  brave_profile_policy_provider_ = provider.get();                            \
  policy_providers_.push_back(provider.get());                                \
  provider->Init(schema_registry);                                            \
  wrapped_policy_providers_.push_back(std::move(provider));

#include <chrome/browser/policy/profile_policy_connector.cc>  // IWYU pragma: export

namespace policy {
raw_ptr<policy::ConfigurationPolicyProvider>
ProfilePolicyConnector::GetBraveProfilePolicyProvider() {
  return brave_profile_policy_provider_;
}
}  // namespace policy

#undef BRAVE_PROFILE_POLICY_CONNECTOR_INIT
