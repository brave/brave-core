/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_H_

#include <optional>

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/brave_origin/brave_origin_policy_info.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefService;

namespace policy {
class PolicyService;
}  // namespace policy

namespace brave_origin {

// This keyed service will maintain the definitions/mappings of policies to
// preferences.
//
// This is separate from BraveProfilePolicyProvider which handles the actual
// integration with Chromium's policy framework.
class BraveOriginService : public KeyedService {
 public:
  explicit BraveOriginService(PrefService* local_state,
                              PrefService* profile_prefs,
                              std::string_view profile_id,
                              policy::PolicyService* policy_service);
  ~BraveOriginService() override;

  // Check if a preference is controlled by BraveOrigin
  bool IsPrefControlledByBraveOrigin(std::string_view pref_name) const;

  // Update the BraveOrigin browser-level policy value
  bool SetBrowserPolicyValue(std::string_view pref_name, bool value);

  // Update the BraveOrigin profile-level policy value
  bool SetProfilePolicyValue(std::string_view pref_name, bool value);

  // Get the current value of a BraveOrigin browser preference
  std::optional<bool> GetBrowserPrefValue(std::string_view pref_name) const;

  // Get the current value of a BraveOrigin profile preference
  std::optional<bool> GetProfilePrefValue(std::string_view pref_name) const;

 protected:
  // Local state and profile preferences this state is associated with
  raw_ptr<PrefService> local_state_;
  raw_ptr<PrefService> profile_prefs_;

  // The profile_id is a calculated hash which will be used to look up
  // the policy values for a particular profile.
  std::string profile_id_;
  raw_ptr<policy::PolicyService> policy_service_;
};

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_H_
