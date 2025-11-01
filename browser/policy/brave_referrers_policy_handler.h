/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_POLICY_BRAVE_REFERRERS_POLICY_HANDLER_H_
#define BRAVE_BROWSER_POLICY_BRAVE_REFERRERS_POLICY_HANDLER_H_

#include "components/policy/core/browser/configuration_policy_handler.h"

namespace policy {

// Handles the |policy::key::kDefaultBraveReferrersSetting| policy.
class BraveReferrersPolicyHandler : public IntRangePolicyHandler {
 public:
  BraveReferrersPolicyHandler();
  ~BraveReferrersPolicyHandler() override;

  BraveReferrersPolicyHandler(const BraveReferrersPolicyHandler&) = delete;
  BraveReferrersPolicyHandler& operator=(const BraveReferrersPolicyHandler&) =
      delete;
};

}  // namespace policy

#endif  // BRAVE_BROWSER_POLICY_BRAVE_REFERRERS_POLICY_HANDLER_H_
