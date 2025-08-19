/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_utils.h"
#include "chrome/browser/enterprise/util/managed_browser_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_service.h"
#include "components/policy/core/common/policy_types.h"

#define ShouldDisplayManagedUi ShouldDisplayManagedUi_ChromiumImpl
#include <chrome/browser/ui/managed_ui.cc>
#undef ShouldDisplayManagedUi

namespace {

bool IsManagedOnlyByBraveOrigin(Profile* profile) {
  if (!brave_origin::IsBraveOriginEnabled()) {
    return false;
  }

  if (!enterprise_util::IsBrowserManaged(profile)) {
    return false;
  }

  // Check if any policy has a non-BraveOrigin source
  // If so, then management is not solely due to BraveOrigin
  policy::PolicyService* policy_service =
      profile->GetProfilePolicyConnector()->policy_service();
  if (!policy_service) {
    return false;
  }

  const policy::PolicyMap& policies = policy_service->GetPolicies(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));

  // Check all policies, not just BraveOrigin-related ones
  for (const auto& [policy_name, entry] : policies) {
    if (entry.source != policy::POLICY_SOURCE_BRAVE_ORIGIN) {
      return false;  // Found non-BraveOrigin policy, so not managed only by
                     // BraveOrigin
    }
  }

  return true;  // All policies are from BraveOrigin
}

}  // namespace

// Our override implementation
bool ShouldDisplayManagedUi(Profile* profile) {
  // If managed only by BraveOrigin, don't show the management UI
  if (IsManagedOnlyByBraveOrigin(profile)) {
    return false;
  }

  // Otherwise, use the original logic
  return ShouldDisplayManagedUi_ChromiumImpl(profile);
}
