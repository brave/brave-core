/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_CONFIRMATION_REDEEM_CONFIRMATION_INTERFACE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_CONFIRMATION_REDEEM_CONFIRMATION_INTERFACE_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/redeem_confirmation_delegate.h"

namespace brave_ads {

struct ConfirmationInfo;

class RedeemConfirmationInterface {
 public:
  virtual ~RedeemConfirmationInterface() = default;

  virtual void SetDelegate(
      base::WeakPtr<RedeemConfirmationDelegate> delegate) = 0;

  virtual void Redeem(const ConfirmationInfo& confirmation) = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_CONFIRMATION_REDEEM_CONFIRMATION_INTERFACE_H_
