/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_H_

#include "base/values.h"
#include "components/keyed_service/core/keyed_service.h"

namespace brave_origin {

// This keyed service will maintain the definitions/mappings of policies to
// preferences.
//
// This is separate from BraveProfilePolicyProvider which handles the actual
// integration with Chromium's policy framework.
class BraveOriginService : public KeyedService {
 public:
  explicit BraveOriginService(const std::string& profile_id);
  ~BraveOriginService() override;

  // Check if a preference is controlled by BraveOrigin
  bool IsPrefControlledByBraveOrigin(const std::string& pref_name) const;

  // Update the BraveOrigin policy value for a preference
  bool SetBraveOriginPolicyValue(const std::string& pref_name,
                                 base::Value value);

  // Get the current value of a BraveOrigin preference
  base::Value GetBraveOriginPrefValue(const std::string& pref_name) const;

 protected:
  // The profile_id is a calculated hash which will be used to look up
  // the policy values for a particular profile.
  std::string profile_id_;
};

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_H_
