/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>

#include "chrome/browser/profiles/profile.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_service.h"
#include "components/policy/core/common/policy_types.h"

#define ShouldDisplayManagedUi ShouldDisplayManagedUi_ChromiumImpl
#include <chrome/browser/ui/managed_ui.cc>
#undef ShouldDisplayManagedUi

namespace brave_policy {

bool ShouldHideManagedUI(const policy::PolicyMap& policies) {
  // Empty policy map means no management at all
  if (policies.empty()) {
    return false;
  }

  // Check all policies, not just BraveOrigin-related ones
  return std::ranges::all_of(policies, [](const auto& policy_pair) {
    const auto& [policy_name, entry] = policy_pair;
    return entry.source == policy::POLICY_SOURCE_BRAVE;
  });
}

}  // namespace brave_policy

// Our override implementation
bool ShouldDisplayManagedUi(Profile* profile) {
  if (!ShouldDisplayManagedUi_ChromiumImpl(profile)) {
    return false;
  }

  // Check if we should hide due to Brave-only management
  policy::PolicyService* policy_service =
      profile->GetProfilePolicyConnector()->policy_service();
  if (policy_service) {
    const policy::PolicyMap& policies = policy_service->GetPolicies(
        policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));

    if (brave_policy::ShouldHideManagedUI(policies)) {
      return false;
    }
  }

  return true;
}
