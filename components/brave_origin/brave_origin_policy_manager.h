/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_POLICY_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_POLICY_MANAGER_H_

#include <optional>

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

using PoliciesEnabledMap = base::flat_map<std::string, bool>;

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

  // Initialize with pref definitions mappings from browser layer and local
  // state
  void Init(BraveOriginPolicyMap&& browser_policy_definitions,
            BraveOriginPolicyMap&& profile_policy_definitions,
            PrefService* local_state);

  // Add/remove observers for policy readiness notifications
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Get policy value for a specific pref, using local state value or default
  // For profile-scoped prefs, profile_id is required to construct the correct
  // key. Returns std::nullopt if the pref is unknown.
  std::optional<bool> GetPolicyValue(
      std::string_view pref_name,
      std::optional<std::string_view> profile_id = std::nullopt) const;

  // Get browser-level policy key-value pairs (policy values from local state or
  // defaults)
  PoliciesEnabledMap GetAllBrowserPolicies() const;

  // Get profile-level policy key-value pairs for a specific profile (policy
  // values from local state or defaults)
  PoliciesEnabledMap GetAllProfilePolicies(std::string_view profile_id) const;

  // Set browser-level policy value in local state
  void SetBrowserPolicyValue(std::string_view pref_name, bool value);

  // Set profile-level policy value in local state
  void SetProfilePolicyValue(std::string_view pref_name,
                             bool value,
                             std::string_view profile_id);

  // Check if the singleton has been initialized
  bool IsInitialized() const;

  // Helper function to get pref info from pref definitions
  const BraveOriginPolicyInfo* GetPrefInfo(std::string_view pref_name);

  // Shutdown the policy manager, clearing state and observers
  void Shutdown();

  BraveOriginPolicyManager(const BraveOriginPolicyManager&) = delete;
  BraveOriginPolicyManager& operator=(const BraveOriginPolicyManager&) = delete;

 private:
  friend base::NoDestructor<BraveOriginPolicyManager>;
  friend class BraveOriginPolicyManagerTest;

  BraveOriginPolicyManager();
  ~BraveOriginPolicyManager();

  // Internal helper to get policy value with pre-found policy info and policies
  // dict
  bool GetPolicyValueInternal(
      const BraveOriginPolicyInfo& policy_info,
      const base::Value::Dict& policies_dict,
      std::optional<std::string_view> profile_id = std::nullopt) const;

  // Internal helper to set policy value in local state
  void SetPolicyValueInternal(std::string_view pref_name,
                              bool value,
                              const BraveOriginPolicyMap& policy_definitions,
                              std::optional<std::string_view> profile_id);

  bool initialized_ = false;
  BraveOriginPolicyMap browser_policy_definitions_;
  BraveOriginPolicyMap profile_policy_definitions_;
  raw_ptr<PrefService> local_state_ = nullptr;
  base::ObserverList<Observer> observers_;
};

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_POLICY_MANAGER_H_
