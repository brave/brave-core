/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/profile_id.h"
#include "brave/components/brave_policy/brave_policy_profile_id-forward.inc"
#include "brave/components/brave_policy/brave_profile_policy_provider.h"

namespace brave_policy {

void SetBraveProfilePolicyProviderProfileID(
    policy::ConfigurationPolicyProvider* provider,
    const base::FilePath& profile_path) {
  auto profile_id = ::brave_origin::GetProfileId(profile_path);
  static_cast<BraveProfilePolicyProvider*>(provider)->SetProfileID(profile_id);
}

}  // namespace brave_policy
