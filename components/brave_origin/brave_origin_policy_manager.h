/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_POLICY_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_POLICY_MANAGER_H_

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "brave/components/brave_origin/brave_origin_policy_info.h"

class PrefService;

namespace base {
template <typename T>
class NoDestructor;
}
namespace brave_origin {

// Singleton that holds BraveOrigin preference definitions and manages
// access to policy values from local state. This completely abstracts away
// the local state management from policy providers.
class BraveOriginPolicyManager {
 public:
  // Observer interface for objects that need to be notified when
  // BraveOrigin policies are loaded/changed.
  class Observer : public base::CheckedObserver {
   public:
    // Called when BraveOrigin policies become available or are updated.
    virtual void OnBraveOriginPoliciesReady() = 0;
  };

  static BraveOriginPolicyManager* GetInstance();

  // Initialize with pref definitions mappings from browser layer
  void Init(BraveOriginPolicyMap pref_definitions);

  // Set local state - should be called when local state is
  // available
  void SetLocalState(PrefService* local_state);

  // Add/remove observers for policy readiness notifications
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Get policy value for a specific pref, using local state value or default
  // For profile-scoped prefs, profile_id is required to construct the correct
  // key
  const base::Value& GetPolicyValue(const std::string& pref_name,
                                    const std::string& profile_id = "") const;

  // Get all pref definitions (metadata only)
  const BraveOriginPolicyMap& GetDefinitions() const;

  // Check if the singleton has been initialized
  bool IsInitialized() const;

  // Helper function to get pref info from pref definitions
  const BraveOriginPolicyInfo* GetPrefInfo(const std::string& pref_name);

  // Helper function to construct the policy key for local state lookup
  // For global prefs: returns pref_name
  // For profile prefs: returns "profile_id.pref_name"
  static std::string GetPolicyKey(const BraveOriginPolicyInfo& policy_info,
                                  const std::string& profile_id = "");

  BraveOriginPolicyManager(const BraveOriginPolicyManager&) = delete;
  BraveOriginPolicyManager& operator=(const BraveOriginPolicyManager&) = delete;

 private:
  friend base::NoDestructor<BraveOriginPolicyManager>;

  BraveOriginPolicyManager();
  ~BraveOriginPolicyManager();

  bool initialized_;
  BraveOriginPolicyMap policy_definitions_;
  raw_ptr<PrefService> local_state_ = nullptr;
  base::ObserverList<Observer> observers_;
};

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_POLICY_MANAGER_H_
