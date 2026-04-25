/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_POLICY_HANDLERS_BRAVE_ADBLOCK_POLICY_HANDLER_H_
#define BRAVE_BROWSER_POLICY_HANDLERS_BRAVE_ADBLOCK_POLICY_HANDLER_H_

#include "components/policy/core/browser/configuration_policy_handler.h"

class PrefValueMap;

namespace policy {

class PolicyMap;

// Values for the DefaultBraveAdblockSetting policy. These must be in sync
// with the policy definition DefaultBraveAdblockSetting.yaml.
enum class BraveAdblockSetting {
  kAllowAds = 1,
  kBlockAds = 2,
};

// Handles the |policy::key::kDefaultBraveAdblockSetting| policy.
class BraveAdblockPolicyHandler : public IntRangePolicyHandlerBase {
 public:
  BraveAdblockPolicyHandler();
  ~BraveAdblockPolicyHandler() override;

  BraveAdblockPolicyHandler(const BraveAdblockPolicyHandler&) = delete;
  BraveAdblockPolicyHandler& operator=(const BraveAdblockPolicyHandler&) =
      delete;

 protected:
  // IntRangePolicyHandlerBase:
  void ApplyPolicySettings(const PolicyMap& policies,
                           PrefValueMap* prefs) override;
};

}  // namespace policy

#endif  // BRAVE_BROWSER_POLICY_HANDLERS_BRAVE_ADBLOCK_POLICY_HANDLER_H_
