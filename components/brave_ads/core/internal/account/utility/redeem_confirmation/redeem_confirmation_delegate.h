/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_CONFIRMATION_REDEEM_CONFIRMATION_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_CONFIRMATION_REDEEM_CONFIRMATION_DELEGATE_H_

#include "base/memory/weak_ptr.h"

namespace brave_ads {

struct ConfirmationInfo;

class RedeemConfirmationDelegate
    : public base::SupportsWeakPtr<RedeemConfirmationDelegate> {
 public:
  // Invoked to tell the delegate that the `confirmation` was redeemed.
  virtual void OnDidRedeemConfirmation(const ConfirmationInfo& confirmation) {}

  // Invoked to tell the delegate that `confirmation` redemption failed.
  virtual void OnFailedToRedeemConfirmation(
      const ConfirmationInfo& confirmation,
      bool should_retry) {}

 protected:
  virtual ~RedeemConfirmationDelegate() = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_CONFIRMATION_REDEEM_CONFIRMATION_DELEGATE_H_
