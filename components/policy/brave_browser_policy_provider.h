/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_POLICY_BRAVE_BROWSER_POLICY_PROVIDER_H_
#define BRAVE_COMPONENTS_POLICY_BRAVE_BROWSER_POLICY_PROVIDER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_origin/brave_origin_pref_info.h"
#include "components/policy/core/common/configuration_policy_provider.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace brave_policy {

// Browser-level policy provider for Brave Origin global scope preferences.
// This provider handles policies that affect local state (global) preferences
// and is registered with the BrowserPolicyConnector for machine-wide policy
// management.
class BraveBrowserPolicyProvider : public policy::ConfigurationPolicyProvider {
 public:
  BraveBrowserPolicyProvider();
  ~BraveBrowserPolicyProvider() override;

  BraveBrowserPolicyProvider(const BraveBrowserPolicyProvider&) = delete;
  BraveBrowserPolicyProvider& operator=(const BraveBrowserPolicyProvider&) =
      delete;

  // ConfigurationPolicyProvider implementation.
  void Initialize(PrefService* local_state, policy::SchemaRegistry* registry);
  void Shutdown() override;
  void RefreshPolicies(policy::PolicyFetchReason reason) override;
  bool IsFirstPolicyLoadComplete(policy::PolicyDomain domain) const override;

 private:
  // Loads policies for browser scope preferences only.
  policy::PolicyBundle LoadPolicies();

  // Helper to set policy for a global scope preference
  void LoadBraveOriginPolicies(policy::PolicyBundle& bundle);
  void LoadBraveOriginPolicy(
      policy::PolicyMap& policy_map,
      const base::Value::Dict& policies_dict,
      const brave_origin::BraveOriginPrefInfo& pref_info);

  // Callback for when brave_origin_policies pref changes
  void OnBraveOriginPoliciesChanged();

  bool first_policies_loaded_;
  raw_ptr<PrefService> local_state_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;

  base::WeakPtrFactory<BraveBrowserPolicyProvider> weak_factory_{this};
};

}  // namespace brave_policy

#endif  // BRAVE_COMPONENTS_POLICY_BRAVE_BROWSER_POLICY_PROVIDER_H_
