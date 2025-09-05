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

namespace policy {
class PolicyService;
}  // namespace policy

namespace brave_origin {

// Manages the business logic and state for Brave Origin feature detection.
// This keyed service determines whether a user qualifies for Brave Origin
// policies and maintains the definitions/mappings of policies to preferences.
//
// This is separate from BraveProfilePolicyProvider which handles the actual
// integration with Chromium's policy framework.
class BraveOriginService : public KeyedService {
 public:
  BraveOriginService(PrefService* local_state,
                     PrefService* profile_prefs,
                     const std::string& profile_id,
                     policy::PolicyService* policy_service);

  // Check if a preference is controlled by BraveOrigin
  bool IsPrefControlledByBraveOrigin(const std::string& pref_name) const;

  // Update the BraveOrigin policy value for a preference (for mojom interface)
  bool SetBraveOriginPolicyValue(const std::string& pref_name,
                                 base::Value value);

  // Get the current value of a BraveOrigin preference (for mojom interface)
  base::Value GetBraveOriginPrefValue(const std::string& pref_name) const;

  // Local state policy methods
  void UpdateLocalStatePolicyPref(const BraveOriginPrefInfo* pref_info,
                                  const base::Value& value);

  // KeyedService overrides
  void Shutdown() override;

  ~BraveOriginService() override;

 private:
  // Local state and profile preferences this state is associated with
  raw_ptr<PrefService> local_state_;
  raw_ptr<PrefService> profile_prefs_;

  // Profile identifier for local state key scoping
  std::string profile_id_;

  // Policy service for triggering policy refresh operations
  raw_ptr<policy::PolicyService> policy_service_;

  // Policy definitions for this service instance
  // BraveOriginPrefMap pref_definitions_;
};

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_H_
