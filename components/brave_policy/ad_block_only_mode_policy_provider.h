/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_POLICY_AD_BLOCK_ONLY_MODE_POLICY_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_POLICY_AD_BLOCK_ONLY_MODE_POLICY_PROVIDER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace policy {
class ConfigurationPolicyProvider;
class PolicyBundle;
}  // namespace policy

namespace brave_policy {

class AdBlockOnlyModePolicyProvider {
 public:
  AdBlockOnlyModePolicyProvider(
      PrefService* local_state,
      policy::ConfigurationPolicyProvider& policy_provider);
  virtual ~AdBlockOnlyModePolicyProvider();

  AdBlockOnlyModePolicyProvider(const AdBlockOnlyModePolicyProvider&) = delete;
  AdBlockOnlyModePolicyProvider& operator=(
      const AdBlockOnlyModePolicyProvider&) = delete;

  void Init();

  void MaybeLoadPolicies(policy::PolicyBundle& bundle);

 private:
  void OnAdBlockOnlyModeChanged();

  const raw_ptr<PrefService> local_state_;

  const raw_ref<policy::ConfigurationPolicyProvider> policy_provider_;

  PrefChangeRegistrar pref_change_registrar_;

  base::WeakPtrFactory<AdBlockOnlyModePolicyProvider> weak_factory_{this};
};

}  // namespace brave_policy

#endif  // BRAVE_COMPONENTS_BRAVE_POLICY_AD_BLOCK_ONLY_MODE_POLICY_PROVIDER_H_
