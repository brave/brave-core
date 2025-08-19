/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_H_

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/brave_origin/brave_origin_pref_info.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefService;
class PrefRegistrySimple;

namespace policy {
enum class PolicyFetchReason;
class PolicyService;
}  // namespace policy

namespace brave_origin {

namespace prefs {
// Base key for all profile-scoped BraveOrigin policy preferences in local state
inline constexpr char kBravePolicies[] = "brave_policies";
// Suffix for tracking external management of policies
inline constexpr char kExternallyManagedSuffix[] = ".externally_managed";
}  // namespace prefs

// Manages the business logic and state for Brave Origin feature detection.
// This keyed service determines whether a user qualifies for Brave Origin
// policies and maintains the definitions/mappings of policies to preferences.
//
// This is separate from BraveOriginPolicyProvider which handles the actual
// integration with Chromium's policy framework.
class BraveOriginService : public KeyedService {
 public:
  BraveOriginService(PrefService* local_state,
                     PrefService* profile_prefs,
                     const std::string& profile_id,
                     BraveOriginPrefMap pref_definitions,
                     policy::PolicyService* policy_service);

  // Returns true if the user is considered a Brave Origin user.
  bool IsBraveOriginUser() const;

  // Check if a preference is controlled by BraveOrigin
  bool IsPrefControlledByBraveOrigin(const std::string& pref_name) const;

  // Check if the browser was managed before BraveOrigin policies.
  // This is called by chromium_src override which determines if the UI should
  // be shown to let the user know their browser is managed.
  bool WasManagedBeforeBraveOrigin() const;

  // Update the BraveOrigin policy value for a preference (for mojom interface)
  bool SetBraveOriginPolicyValue(const std::string& pref_name,
                                 base::Value value);

  // Get the current value of a BraveOrigin preference (for mojom interface)
  base::Value GetBraveOriginPrefValue(const std::string& pref_name) const;

  // Get preference info by preference name
  const BraveOriginPrefInfo* GetPrefInfo(const std::string& pref_name) const;

  // Local state policy methods
  static std::string GetLocalStatePolicyPrefKey(const std::string& profile_id,
                                                const std::string& pref_name);
  void UpdateLocalStatePolicyPref(const std::string& pref_name,
                                  const base::Value& value);

  // Register local state preferences
  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

  // KeyedService overrides
  void Shutdown() override;

  ~BraveOriginService() override;

 private:
  // Initialize local state policy preferences for this profile
  void InitializeLocalStatePolicyPrefs();

  // Local state and profile preferences this state is associated with
  raw_ptr<PrefService> local_state_;
  raw_ptr<PrefService> profile_prefs_;

  // Profile identifier for local state key scoping
  std::string profile_id_;

  // Policy service for triggering policy refresh operations
  raw_ptr<policy::PolicyService> policy_service_;

  // Policy definitions for this service instance
  BraveOriginPrefMap pref_definitions_;
};

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_H_
