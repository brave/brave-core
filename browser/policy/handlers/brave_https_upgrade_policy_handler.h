/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_POLICY_HANDLERS_BRAVE_HTTPS_UPGRADE_POLICY_HANDLER_H_
#define BRAVE_BROWSER_POLICY_HANDLERS_BRAVE_HTTPS_UPGRADE_POLICY_HANDLER_H_

#include "components/policy/core/browser/configuration_policy_handler.h"

namespace policy {

// Values for the DefaultBraveHttpsUpgradeSetting policy. These must be in sync
// with the policy definition DefaultBraveHttpsUpgradeSetting.yaml.
enum class BraveHttpsUpgradeSetting {
  kDisabled = 1,
  kStrict = 2,
  kStandard = 3,
};

// Handles the |policy::key::kDefaultBraveHttpsUpgradeSetting| policy.
class BraveHttpsUpgradePolicyHandler : public IntRangePolicyHandlerBase {
 public:
  BraveHttpsUpgradePolicyHandler();
  ~BraveHttpsUpgradePolicyHandler() override;

  BraveHttpsUpgradePolicyHandler(const BraveHttpsUpgradePolicyHandler&) =
      delete;
  BraveHttpsUpgradePolicyHandler& operator=(
      const BraveHttpsUpgradePolicyHandler&) = delete;

 protected:
  // IntRangePolicyHandlerBase:
  void ApplyPolicySettings(const PolicyMap& policies,
                           PrefValueMap* prefs) override;
};

}  // namespace policy

#endif  // BRAVE_BROWSER_POLICY_HANDLERS_BRAVE_HTTPS_UPGRADE_POLICY_HANDLER_H_
