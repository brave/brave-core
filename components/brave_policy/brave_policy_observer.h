/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_POLICY_BRAVE_POLICY_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_POLICY_BRAVE_POLICY_OBSERVER_H_

#include <string_view>

#include "base/observer_list_types.h"

namespace brave_policy {

// Observer interface for objects that need to be notified when
// Brave policies are loaded/changed.
class BravePolicyObserver : public base::CheckedObserver {
 public:
  // Called when Brave policies become available or are updated.
  virtual void OnBravePoliciesReady() = 0;

  // Called when a browser-level policy preference is changed.
  virtual void OnBrowserPolicyChanged(std::string_view policy_key) {}

  // Called when a profile-level policy preference is changed.
  virtual void OnProfilePolicyChanged(std::string_view policy_key,
                                      std::string_view profile_id) {}
};

}  // namespace brave_policy

#endif  // BRAVE_COMPONENTS_BRAVE_POLICY_BRAVE_POLICY_OBSERVER_H_
