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
#include "components/policy/core/common/configuration_policy_provider.h"

namespace brave_policy {

// Policy provider for profile level polciies.
// Note: When this is created, the profile is not yet initialized.
class BraveProfilePolicyProvider : public policy::ConfigurationPolicyProvider {
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

 private:
  policy::PolicyBundle LoadPolicies();

  bool first_policies_loaded_ = false;

  base::WeakPtrFactory<BraveProfilePolicyProvider> weak_factory_{this};
};

}  // namespace brave_policy

#endif  // BRAVE_COMPONENTS_BRAVE_POLICY_BRAVE_PROFILE_POLICY_PROVIDER_H_
