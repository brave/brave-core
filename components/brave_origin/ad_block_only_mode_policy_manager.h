/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_AD_BLOCK_ONLY_MODE_POLICY_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_AD_BLOCK_ONLY_MODE_POLICY_MANAGER_H_

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "components/prefs/pref_change_registrar.h"

namespace base {
template <typename T>
class NoDestructor;
}

class PrefService;

namespace brave_origin {

// Singleton that holds Ad Block Only mode preference handling and manages
// setting policy values when the preference is changed. This
// abstracts away the local state management from policy provider.
// TODO(https://github.com/brave/brave-browser/issues/50077): Refactor this
// class when `BravePolicyManager` is introduced.
class AdBlockOnlyModePolicyManager final {
 public:
  // Observer interface for objects that need to be notified when
  // Ad Block Only mode need to be refreshed.
  class Observer : public base::CheckedObserver {
   public:
    // Called when Ad Block Only mode policies need to be refreshed due to
    // preference changes.
    virtual void OnAdBlockOnlyModePoliciesChanged() = 0;
  };

  static AdBlockOnlyModePolicyManager* GetInstance();

  void Init(PrefService* local_state);
  void Shutdown();
  bool IsInitialized() const;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  bool IsAdBlockOnlyModeEnabled() const;

  AdBlockOnlyModePolicyManager(const AdBlockOnlyModePolicyManager&) = delete;
  AdBlockOnlyModePolicyManager& operator=(const AdBlockOnlyModePolicyManager&) =
      delete;

 private:
  friend base::NoDestructor<AdBlockOnlyModePolicyManager>;

  void OnAdBlockOnlyModeChanged();

  AdBlockOnlyModePolicyManager();
  ~AdBlockOnlyModePolicyManager();

  bool initialized_ = false;
  raw_ptr<PrefService> local_state_;  // Not owned.
  PrefChangeRegistrar pref_change_registrar_;
  base::ObserverList<Observer> observers_;
};

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_AD_BLOCK_ONLY_MODE_POLICY_MANAGER_H_
