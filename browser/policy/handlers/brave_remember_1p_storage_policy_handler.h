/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_POLICY_HANDLERS_BRAVE_REMEMBER_1P_STORAGE_POLICY_HANDLER_H_
#define BRAVE_BROWSER_POLICY_HANDLERS_BRAVE_REMEMBER_1P_STORAGE_POLICY_HANDLER_H_

#include "components/policy/core/browser/configuration_policy_handler.h"

class PrefValueMap;

namespace policy {

class PolicyMap;

// Values for the DefaultBraveRemember1PStorageSetting policy. These must be in
// sync with the policy definition DefaultBraveRemember1PStorageSetting.yaml.
enum class BraveRemember1PStorageSetting {
  kRememberFirstPartyStorage = 1,
  kForgetFirstPartyStorage = 2,
};

// Handles the |policy::key::kDefaultBraveRemember1PStorageSetting| policy.
class BraveRemember1PStoragePolicyHandler : public IntRangePolicyHandlerBase {
 public:
  BraveRemember1PStoragePolicyHandler();
  ~BraveRemember1PStoragePolicyHandler() override;

  BraveRemember1PStoragePolicyHandler(
      const BraveRemember1PStoragePolicyHandler&) = delete;
  BraveRemember1PStoragePolicyHandler& operator=(
      const BraveRemember1PStoragePolicyHandler&) = delete;

 protected:
  // IntRangePolicyHandlerBase:
  void ApplyPolicySettings(const PolicyMap& policies,
                           PrefValueMap* prefs) override;
};

}  // namespace policy

#endif  // BRAVE_BROWSER_POLICY_HANDLERS_BRAVE_REMEMBER_1P_STORAGE_POLICY_HANDLER_H_
