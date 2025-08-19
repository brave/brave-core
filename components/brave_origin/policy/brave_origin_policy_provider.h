/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_POLICY_BRAVE_ORIGIN_POLICY_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_POLICY_BRAVE_ORIGIN_POLICY_PROVIDER_H_

#include "base/containers/flat_set.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/values.h"
#include "brave/components/brave_origin/brave_origin_pref_info.h"
#include "components/policy/core/common/configuration_policy_provider.h"

class PrefService;
namespace brave_origin {

// Policy framework adapter for Brave Origin users. This provider integrates
// with Chromium's policy system to supply policies when BraveOriginService
// determines the user qualifies as a Brave Origin user.
//
// BraveOrigin policy provider that reads from local state preferences.
// Uses profile-scoped local state keys to avoid needing Profile access.
// When this is created, the profile is not yet initialized.
class BraveOriginPolicyProvider : public policy::ConfigurationPolicyProvider {
 public:
  explicit BraveOriginPolicyProvider(PrefService* local_state);
  ~BraveOriginPolicyProvider() override;

  BraveOriginPolicyProvider(const BraveOriginPolicyProvider&) = delete;
  BraveOriginPolicyProvider& operator=(const BraveOriginPolicyProvider&) =
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
  void LoadPolicyForPref(policy::PolicyMap& policy_map,
                         const BraveOriginPrefInfo& pref_info);

  std::string profile_id_;
  bool first_policies_loaded_;
  raw_ptr<PrefService> local_state_;

  base::WeakPtrFactory<BraveOriginPolicyProvider> weak_factory_{this};
};

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_POLICY_BRAVE_ORIGIN_POLICY_PROVIDER_H_
