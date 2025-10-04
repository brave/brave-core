/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_ONLY_MODE_AD_BLOCK_ONLY_MODE_POLICIES_LOADER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_ONLY_MODE_AD_BLOCK_ONLY_MODE_POLICIES_LOADER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace policy {
class ConfigurationPolicyProvider;
class PolicyBundle;
}  // namespace policy

namespace brave_shields {

// Loads policy overrides for Ad Block Only mode. This experimental feature
// improves website compatibility by keeping ad blocking active while disabling
// certain privacy protections.
class AdBlockOnlyModePoliciesLoader {
 public:
  AdBlockOnlyModePoliciesLoader(
      PrefService* local_state,
      policy::ConfigurationPolicyProvider& policy_provider);
  virtual ~AdBlockOnlyModePoliciesLoader();

  AdBlockOnlyModePoliciesLoader(const AdBlockOnlyModePoliciesLoader&) = delete;
  AdBlockOnlyModePoliciesLoader& operator=(
      const AdBlockOnlyModePoliciesLoader&) = delete;

  void Init();

  void MaybeLoadPolicies(policy::PolicyBundle& bundle);

 private:
  void OnAdBlockOnlyModeChanged();

  const raw_ptr<PrefService> local_state_;  // Not owned.

  const raw_ref<policy::ConfigurationPolicyProvider>
      policy_provider_;  // Not owned, must outlive this class.

  PrefChangeRegistrar pref_change_registrar_;

  base::WeakPtrFactory<AdBlockOnlyModePoliciesLoader> weak_factory_{this};
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_ONLY_MODE_AD_BLOCK_ONLY_MODE_POLICIES_LOADER_H_
