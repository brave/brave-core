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

    // Called when a browser-level policy preference is changed.
    virtual void OnBrowserPolicyChanged(std::string_view policy_key) {}

    // Called when a profile-level policy preference is changed.
    virtual void OnProfilePolicyChanged(std::string_view policy_key,
                                        std::string_view profile_id) {}
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

  // Get policy value for a specific policy
  std::optional<bool> GetPolicyValue(
      std::string_view policy_key,
      std::optional<std::string_view> profile_id = std::nullopt) const;

  // Get browser-level policy key-value pairs (policy values from local state or
  // defaults)
  PoliciesEnabledMap GetAllBrowserPolicies() const;

  // Get profile-level policy key-value pairs for a specific profile (policy
  // values from local state or defaults)
  PoliciesEnabledMap GetAllProfilePolicies(std::string_view profile_id) const;

  // Set a policy value in local state
  void SetPolicyValue(
      std::string_view policy_key,
      bool value,
      std::optional<std::string_view> profile_id = std::nullopt);

  // Determines if the policy is a browser-level policy
  bool IsBrowserPolicy(std::string_view policy_key) const;

  // Determines if the policy is a profile-level policy
  bool IsProfilePolicy(std::string_view policy_key) const;

  // Check if the singleton has been initialized
  bool IsInitialized() const;

  // Helper function to get policy info from policy definitions
  const BraveOriginPolicyInfo* GetPolicyInfo(std::string_view policy_key) const;

  // Shutdown the policy manager, clearing state and observers
  void Shutdown();

  BraveOriginPolicyManager(const BraveOriginPolicyManager&) = delete;
  BraveOriginPolicyManager& operator=(const BraveOriginPolicyManager&) = delete;

 private:
  friend base::NoDestructor<BraveOriginPolicyManager>;
  friend class BraveOriginPolicyManagerTest;

  BraveOriginPolicyManager();
  ~BraveOriginPolicyManager();

  // Internal helper to get policy value with policy key and default value
  bool GetPolicyValueInternal(std::string_view policy_key,
                              bool default_value,
                              const base::Value::Dict& policies_dict,
                              std::optional<std::string_view> profile_id) const;

  bool initialized_ = false;
  BraveOriginPolicyMap browser_policy_definitions_;
  BraveOriginPolicyMap profile_policy_definitions_;
  raw_ptr<PrefService> local_state_ = nullptr;
  base::ObserverList<Observer> observers_;
};

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_POLICY_MANAGER_H_
