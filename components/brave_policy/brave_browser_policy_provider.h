/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_POLICY_BRAVE_BROWSER_POLICY_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_POLICY_BRAVE_BROWSER_POLICY_PROVIDER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_origin/brave_origin_policy_info.h"
#include "brave/components/brave_origin/brave_origin_policy_manager.h"
#include "brave/components/brave_policy/brave_policy_observer.h"
#include "components/policy/core/common/configuration_policy_provider.h"

namespace brave_policy {

// Browser-level policy provider for global scope preferences.
// This provider handles policies that affect global preferences
// and is registered with the BrowserPolicyConnector for machine-wide policy
// management.
class BraveBrowserPolicyProvider : public policy::ConfigurationPolicyProvider,
                                   public BravePolicyObserver {
 public:
  BraveBrowserPolicyProvider();
  ~BraveBrowserPolicyProvider() override;

  BraveBrowserPolicyProvider(const BraveBrowserPolicyProvider&) = delete;
  BraveBrowserPolicyProvider& operator=(const BraveBrowserPolicyProvider&) =
      delete;

  // ConfigurationPolicyProvider implementation.
  void Init(policy::SchemaRegistry* registry) override;
  void RefreshPolicies(policy::PolicyFetchReason reason) override;
  bool IsFirstPolicyLoadComplete(policy::PolicyDomain domain) const override;

  // BravePolicyObserver implementation.
  void OnBravePoliciesReady() override;
  void OnBrowserPolicyChanged(std::string_view policy_key) override;

 private:
  // Loads policies for browser scope preferences only.
  policy::PolicyBundle LoadPolicies();

  // Helper to set BraveOrigin policy for a specific preference
  void LoadBraveOriginPolicies(policy::PolicyBundle& bundle);
  void LoadBraveOriginPolicy(policy::PolicyMap& policy_map,
                             std::string_view policy_key,
                             bool enabled);

  bool first_policies_loaded_ = false;

  base::ScopedObservation<brave_origin::BraveOriginPolicyManager,
                          BravePolicyObserver>
      policy_manager_observation_{this};

  base::WeakPtrFactory<BraveBrowserPolicyProvider> weak_factory_{this};
};

}  // namespace brave_policy

#endif  // BRAVE_COMPONENTS_BRAVE_POLICY_BRAVE_BROWSER_POLICY_PROVIDER_H_
