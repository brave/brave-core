/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/files/file_path.h"
#include "brave/components/brave_origin/profile_id.h"
#include "brave/components/brave_policy/brave_profile_policy_provider.h"
#include "components/policy/core/common/configuration_policy_provider.h"

namespace brave_policy {

// This is the definition of a declaration a chromium_src for setting
// the profile ID on a profile's BraveProfilePolicyProvider.
// The declaration is in
// chromium_src/chrome/browser/policy/profile_policy_connector_builder.cc
void SetBraveProfilePolicyProviderProfileID(
    policy::ConfigurationPolicyProvider* provider,
    const base::FilePath& profile_path) {
  auto profile_id = ::brave_origin::GetProfileId(profile_path);
  static_cast<BraveProfilePolicyProvider*>(provider)->SetProfileID(profile_id);
}

}  // namespace brave_policy
