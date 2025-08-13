/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_POLICY_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_POLICY_PROVIDER_H_

#include "components/policy/core/common/configuration_policy_provider.h"
#include "components/policy/policy_export.h"

class PrefService;

namespace brave_origin {

// The policy provider for Brave Origin users. This provider supplies
// policies based on whether the user is identified as a Brave Origin user.
class POLICY_EXPORT BraveOriginPolicyProvider
    : public policy::ConfigurationPolicyProvider {
 public:
  explicit BraveOriginPolicyProvider(PrefService* local_state);

  BraveOriginPolicyProvider(const BraveOriginPolicyProvider&) = delete;
  BraveOriginPolicyProvider& operator=(const BraveOriginPolicyProvider&) =
      delete;

  ~BraveOriginPolicyProvider() override;

  // ConfigurationPolicyProvider implementation.
  void RefreshPolicies(policy::PolicyFetchReason reason) override;
  bool IsFirstPolicyLoadComplete(policy::PolicyDomain domain) const override;

 private:
  // Loads policies based on Brave Origin user status and preferences.
  policy::PolicyBundle LoadPolicies();

  bool first_policies_loaded_;
  raw_ptr<PrefService> local_state_;
};

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_POLICY_PROVIDER_H_
