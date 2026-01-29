/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>

#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_service.h"
#include "components/policy/core/common/policy_types.h"

#define ShouldDisplayManagedUi ShouldDisplayManagedUi_ChromiumImpl
#include <chrome/browser/ui/managed_ui.cc>
#undef ShouldDisplayManagedUi

namespace brave_policy {

bool HasOnlyBravePolicies(const policy::PolicyMap& policies) {
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
  auto* profile_policy_service =
      profile->GetProfilePolicyConnector()->policy_service();
  if (profile_policy_service) {
    const auto& profile_policies = profile_policy_service->GetPolicies(
        policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));

    if (!brave_policy::HasOnlyBravePolicies(profile_policies)) {
      return true;
    }
  }

  // Also check browser-level policies
  auto* browser_policy_service = g_browser_process->policy_service();
  if (browser_policy_service) {
    const auto& browser_policies = browser_policy_service->GetPolicies(
        policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));

    if (!brave_policy::HasOnlyBravePolicies(browser_policies)) {
      return true;
    }
  }

  // Both profile and browser policies are Brave-only (or empty), so hide
  return false;
}
