/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_POLICY_BRAVE_PROFILE_POLICY_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_POLICY_BRAVE_PROFILE_POLICY_PROVIDER_H_

#include "base/containers/flat_set.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/values.h"
#include "brave/components/brave_origin/ad_block_only_mode_policy_manager.h"
#include "brave/components/brave_origin/brave_origin_policy_info.h"
#include "brave/components/brave_origin/brave_origin_policy_manager.h"
#include "components/policy/core/common/configuration_policy_provider.h"

namespace brave_policy {

// Policy provider for profile level policies.
// Note: When this is created, the profile is not yet initialized.
class BraveProfilePolicyProvider
    : public policy::ConfigurationPolicyProvider,
      public brave_origin::BraveOriginPolicyManager::Observer,
      public brave_origin::AdBlockOnlyModePolicyManager::Observer {
 public:
  BraveProfilePolicyProvider();
  ~BraveProfilePolicyProvider() override;

  BraveProfilePolicyProvider(const BraveProfilePolicyProvider&) = delete;
  BraveProfilePolicyProvider& operator=(const BraveProfilePolicyProvider&) =
      delete;

  // ConfigurationPolicyProvider implementation.
  void Init(policy::SchemaRegistry* registry) override;
  void RefreshPolicies(policy::PolicyFetchReason reason) override;
  bool IsFirstPolicyLoadComplete(policy::PolicyDomain domain) const override;

  // brave_origin::BraveOriginPolicyManager::Observer implementation.
  void OnBraveOriginPoliciesReady() override;
  void OnProfilePolicyChanged(std::string_view policy_key,
                              std::string_view profile_id) override;

  // brave_origin::AdBlockOnlyModePolicyManager::Observer implementation.
  void OnAdBlockOnlyModePoliciesChanged() override;

  void SetProfileID(const std::string& profile_id);

 private:
  policy::PolicyBundle LoadPolicies();

  // Helper to load BraveOrigin profile policies
  void LoadBraveOriginPolicies(policy::PolicyBundle& bundle);
  void LoadBraveOriginPolicy(policy::PolicyMap& bundle_policy_map,
                             std::string_view policy_key,
                             bool enabled);

  void MaybeLoadAdBlockOnlyModePolicies(policy::PolicyBundle& bundle);

  bool first_policies_loaded_ = false;
  bool policies_ready_ = false;
  std::string profile_id_;

  base::ScopedObservation<brave_origin::BraveOriginPolicyManager,
                          brave_origin::BraveOriginPolicyManager::Observer>
      brave_origin_observation_{this};

  base::ScopedObservation<brave_origin::AdBlockOnlyModePolicyManager,
                          brave_origin::AdBlockOnlyModePolicyManager::Observer>
      ad_block_only_mode_policy_manager_observation_{this};

  base::WeakPtrFactory<BraveProfilePolicyProvider> weak_factory_{this};
};

}  // namespace brave_policy

#endif  // BRAVE_COMPONENTS_BRAVE_POLICY_BRAVE_PROFILE_POLICY_PROVIDER_H_
