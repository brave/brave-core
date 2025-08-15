/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_POLICY_BRAVE_ORIGIN_POLICY_PROVIDER_H_
#define BRAVE_BROWSER_POLICY_BRAVE_ORIGIN_POLICY_PROVIDER_H_

#include "components/policy/core/common/configuration_policy_provider.h"
#include "components/policy/policy_export.h"

class PrefRegistrySimple;
class PrefService;

namespace policy {
class PolicyService;
}

namespace brave_origin {

// The policy provider for Brave Origin users. This provider supplies
// policies based on whether the user is identified as a Brave Origin user.
class POLICY_EXPORT BraveOriginPolicyProvider
    : public policy::ConfigurationPolicyProvider {
 public:
  explicit BraveOriginPolicyProvider(PrefService* local_state,
                                     policy::PolicyService* policy_service);
  ~BraveOriginPolicyProvider() override;

  BraveOriginPolicyProvider(const BraveOriginPolicyProvider&) = delete;
  BraveOriginPolicyProvider& operator=(const BraveOriginPolicyProvider&) =
      delete;

  // ConfigurationPolicyProvider implementation.
  void RefreshPolicies(policy::PolicyFetchReason reason) override;

  // Registers local state preferences.
  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);
  bool IsFirstPolicyLoadComplete(policy::PolicyDomain domain) const override;

 private:
  // Loads policies based on Brave Origin user status and preferences.
  policy::PolicyBundle LoadPolicies();

  // Checks if a policy is already set by external providers (not BraveOrigin)
  bool IsPolicySetByExternalProvider(const std::string& policy_key) const;

  // Helper to consistently set BraveOrigin policies with user preference
  // support
  void SetBraveOriginPolicyWithPreference(
      policy::PolicyMap& policy_map,
      const base::Value::Dict& policy_settings,
      const std::string& policy_key,
      const std::string& pref_name,
      const base::Value& default_value);

  bool first_policies_loaded_;
  raw_ptr<PrefService> local_state_;
  raw_ptr<policy::PolicyService> policy_service_;
};

}  // namespace brave_origin

#endif  // BRAVE_BROWSER_POLICY_BRAVE_ORIGIN_POLICY_PROVIDER_H_
