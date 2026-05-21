/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_POLICY_HANDLERS_BRAVE_ADS_ENABLED_POLICY_HANDLER_H_
#define BRAVE_COMPONENTS_POLICY_HANDLERS_BRAVE_ADS_ENABLED_POLICY_HANDLER_H_

#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "components/policy/core/browser/configuration_policy_handler.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_ADS));

class PrefValueMap;

namespace policy {

class PolicyMap;

// Handles the `policy::key::kBraveAdsEnabled` policy. Setting the policy to
// `false` forces all brave ads types prefs to false.
class BraveAdsEnabledPolicyHandler final : public TypeCheckingPolicyHandler {
 public:
  BraveAdsEnabledPolicyHandler();
  BraveAdsEnabledPolicyHandler(const BraveAdsEnabledPolicyHandler&) = delete;
  BraveAdsEnabledPolicyHandler& operator=(const BraveAdsEnabledPolicyHandler&) =
      delete;
  ~BraveAdsEnabledPolicyHandler() override;

  // TypeCheckingPolicyHandler:
  void ApplyPolicySettings(const PolicyMap& policies,
                           PrefValueMap* prefs) override;
};

}  // namespace policy

#endif  // BRAVE_COMPONENTS_POLICY_HANDLERS_BRAVE_ADS_ENABLED_POLICY_HANDLER_H_
