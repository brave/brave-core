/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_POLICY_BRAVE_PROFILE_POLICY_PROVIDER_H_
#define BRAVE_COMPONENTS_POLICY_BRAVE_PROFILE_POLICY_PROVIDER_H_

#include "base/containers/flat_set.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/values.h"
#include "brave/components/brave_origin/brave_origin_pref_info.h"
#include "components/policy/core/common/configuration_policy_provider.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;
namespace brave_policy {

// Policy framework adapter for Brave Origin users. This provider integrates
// with Chromium's policy system to supply policies when BraveOriginService
// determines the user qualifies as a Brave Origin user.
//
// BraveOrigin policy provider that reads from local state preferences.
// Uses profile-scoped local state keys to avoid needing Profile access.
// When this is created, the profile is not yet initialized.
class BraveProfilePolicyProvider : public policy::ConfigurationPolicyProvider {
 public:
  explicit BraveProfilePolicyProvider(PrefService* local_state);
  ~BraveProfilePolicyProvider() override;

  BraveProfilePolicyProvider(const BraveProfilePolicyProvider&) = delete;
  BraveProfilePolicyProvider& operator=(const BraveProfilePolicyProvider&) =
      delete;

  // ConfigurationPolicyProvider implementation.
  void Initialize(const std::string& profile_id,
                  policy::SchemaRegistry* registry);
  void Shutdown() override;
  void RefreshPolicies(policy::PolicyFetchReason reason) override;
  bool IsFirstPolicyLoadComplete(policy::PolicyDomain domain) const override;

 private:
  // Loads policies based on Brave Origin user status and preferences.
  policy::PolicyBundle LoadPolicies();

  // Helper to set BraveOrigin policy for a specific preference
  void LoadBraveOriginPolicies(policy::PolicyBundle& bundle);
  void LoadBraveOriginPolicy(
      policy::PolicyMap& policy_map,
      const base::Value::Dict& policies_dict,
      const brave_origin::BraveOriginPrefInfo& pref_info);

  // Callback for when brave_origin_policies pref changes
  void OnBraveOriginPoliciesChanged();

  std::string profile_id_;
  bool first_policies_loaded_;
  raw_ptr<PrefService> local_state_;
  PrefChangeRegistrar pref_change_registrar_;

  base::WeakPtrFactory<BraveProfilePolicyProvider> weak_factory_{this};
};

}  // namespace brave_policy

#endif  // BRAVE_COMPONENTS_POLICY_BRAVE_PROFILE_POLICY_PROVIDER_H_
