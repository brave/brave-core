/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_POLICY_AD_BLOCK_ONLY_MODE_AD_BLOCK_ONLY_MODE_POLICY_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_POLICY_AD_BLOCK_ONLY_MODE_AD_BLOCK_ONLY_MODE_POLICY_MANAGER_H_

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/values.h"
#include "brave/components/brave_policy/brave_policy_observer.h"
#include "components/prefs/pref_change_registrar.h"

namespace base {
template <typename T>
class NoDestructor;
}

class PrefService;

namespace brave_policy {

using AdBlockOnlyModePolicies = base::flat_map<std::string, base::Value>;

// Singleton that holds Ad Block Only mode preference handling and manages
// setting policy values when the preference is changed. This
// abstracts away the local state management from policy provider.
// TODO(https://github.com/brave/brave-browser/issues/50077): Refactor this
// class when `BravePolicyManager` is introduced.
class AdBlockOnlyModePolicyManager final {
 public:
  static AdBlockOnlyModePolicyManager* GetInstance();

  void Init(PrefService* local_state);
  void Shutdown();

  void AddObserver(BravePolicyObserver* observer);
  void RemoveObserver(BravePolicyObserver* observer);

  AdBlockOnlyModePolicies GetPolicies() const;

  AdBlockOnlyModePolicyManager(const AdBlockOnlyModePolicyManager&) = delete;
  AdBlockOnlyModePolicyManager& operator=(const AdBlockOnlyModePolicyManager&) =
      delete;

 private:
  friend base::NoDestructor<AdBlockOnlyModePolicyManager>;
  AdBlockOnlyModePolicyManager();
  ~AdBlockOnlyModePolicyManager();

  void OnAdBlockOnlyModeChanged();
  AdBlockOnlyModePolicies GetPoliciesImpl() const;

  raw_ptr<PrefService> local_state_;  // Not owned.
  PrefChangeRegistrar pref_change_registrar_;
  base::ObserverList<BravePolicyObserver> observers_;
};

}  // namespace brave_policy

#endif  // BRAVE_COMPONENTS_BRAVE_POLICY_AD_BLOCK_ONLY_MODE_AD_BLOCK_ONLY_MODE_POLICY_MANAGER_H_
