/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_POLICY_BRAVE_BROWSER_POLICY_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_POLICY_BRAVE_BROWSER_POLICY_PROVIDER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "components/policy/core/common/configuration_policy_provider.h"

namespace brave_policy {

// Browser-level policy provider for global scope preferences.
// This provider handles policies that affect global preferences
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
  void Init(policy::SchemaRegistry* registry) override;
  void RefreshPolicies(policy::PolicyFetchReason reason) override;
  bool IsFirstPolicyLoadComplete(policy::PolicyDomain domain) const override;

 private:
  // Loads policies for browser scope preferences only.
  policy::PolicyBundle LoadPolicies();

  bool first_policies_loaded_ = false;

  base::WeakPtrFactory<BraveBrowserPolicyProvider> weak_factory_{this};
};

}  // namespace brave_policy

#endif  // BRAVE_COMPONENTS_BRAVE_POLICY_BRAVE_BROWSER_POLICY_PROVIDER_H_
