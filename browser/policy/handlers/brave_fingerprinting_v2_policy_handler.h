/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_POLICY_HANDLERS_BRAVE_FINGERPRINTING_V2_POLICY_HANDLER_H_
#define BRAVE_BROWSER_POLICY_HANDLERS_BRAVE_FINGERPRINTING_V2_POLICY_HANDLER_H_

#include "components/policy/core/browser/configuration_policy_handler.h"

namespace policy {

// Values for the DefaultBraveFingerprintingV2Setting policy. These must be in
// sync with the policy definition DefaultBraveFingerprintingV2Setting.yaml.
// Value 2 is deprecated Strict Fingerprinting mode and will be ignored if set.
enum class BraveFingerprintingV2Setting {
  kDisableFingerprintingProtection = 1,
  kEnableFingerprintingProtectionStandardMode = 3,
};

// Handles the |policy::key::kDefaultBraveFingerprintingV2Setting| policy.
class BraveFingerprintingV2PolicyHandler : public IntRangePolicyHandlerBase {
 public:
  BraveFingerprintingV2PolicyHandler();
  ~BraveFingerprintingV2PolicyHandler() override;

  BraveFingerprintingV2PolicyHandler(
      const BraveFingerprintingV2PolicyHandler&) = delete;
  BraveFingerprintingV2PolicyHandler& operator=(
      const BraveFingerprintingV2PolicyHandler&) = delete;

 protected:
  // IntRangePolicyHandlerBase:
  void ApplyPolicySettings(const PolicyMap& policies,
                           PrefValueMap* prefs) override;
};

}  // namespace policy

#endif  // BRAVE_BROWSER_POLICY_HANDLERS_BRAVE_FINGERPRINTING_V2_POLICY_HANDLER_H_
