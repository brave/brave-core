/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_POLICY_HANDLERS_BRAVE_REFERRERS_POLICY_HANDLER_H_
#define BRAVE_BROWSER_POLICY_HANDLERS_BRAVE_REFERRERS_POLICY_HANDLER_H_

#include "components/policy/core/browser/configuration_policy_handler.h"

class PrefValueMap;

namespace policy {

class PolicyMap;

// Values for the DefaultBraveReferrersSetting policy. These must be in sync
// with the policy definition DefaultBraveReferrersSetting.yaml.
enum class BraveReferrersSetting {
  kAllowPermissiveReferrerPolicy = 1,
  kCapToStrictReferrerPolicy = 2,
};

// Handles the |policy::key::kDefaultBraveReferrersSetting| policy.
class BraveReferrersPolicyHandler : public IntRangePolicyHandlerBase {
 public:
  BraveReferrersPolicyHandler();
  ~BraveReferrersPolicyHandler() override;

  BraveReferrersPolicyHandler(const BraveReferrersPolicyHandler&) = delete;
  BraveReferrersPolicyHandler& operator=(const BraveReferrersPolicyHandler&) =
      delete;

 protected:
  // IntRangePolicyHandlerBase:
  void ApplyPolicySettings(const PolicyMap& policies,
                           PrefValueMap* prefs) override;
};

}  // namespace policy

#endif  // BRAVE_BROWSER_POLICY_HANDLERS_BRAVE_REFERRERS_POLICY_HANDLER_H_
