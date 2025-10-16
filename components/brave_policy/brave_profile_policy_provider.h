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
#include "brave/components/brave_origin/brave_origin_policy_info.h"
#include "brave/components/brave_origin/brave_origin_policy_manager.h"
#include "brave/components/brave_policy/brave_policy_observer.h"
#include "components/policy/core/common/configuration_policy_provider.h"

namespace brave_policy {

// Policy provider for profile level policies.
// Note: When this is created, the profile is not yet initialized.
class BraveProfilePolicyProvider : public policy::ConfigurationPolicyProvider,
                                   public BravePolicyObserver {
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

  // BravePolicyObserver implementation.
  void OnBravePoliciesReady() override;
  void OnProfilePolicyChanged(std::string_view policy_key,
                              std::string_view profile_id) override;
  void SetProfileID(const std::string& profile_id);

 private:
  policy::PolicyBundle LoadPolicies();

  // Helper to load BraveOrigin profile policies
  void LoadBraveOriginPolicies(policy::PolicyBundle& bundle);
  void LoadBraveOriginPolicy(policy::PolicyMap& bundle_policy_map,
                             std::string_view policy_key,
                             bool enabled);

  bool first_policies_loaded_ = false;
  bool policies_ready_ = false;
  std::string profile_id_;

  base::ScopedObservation<brave_origin::BraveOriginPolicyManager,
                          BravePolicyObserver>
      brave_origin_observation_{this};

  base::WeakPtrFactory<BraveProfilePolicyProvider> weak_factory_{this};
};

}  // namespace brave_policy

#endif  // BRAVE_COMPONENTS_BRAVE_POLICY_BRAVE_PROFILE_POLICY_PROVIDER_H_
