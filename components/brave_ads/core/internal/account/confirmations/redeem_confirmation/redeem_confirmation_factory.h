/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_CONFIRMATION_REDEEM_CONFIRMATION_FACTORY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_CONFIRMATION_REDEEM_CONFIRMATION_FACTORY_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/redeem_confirmation/redeem_confirmation_delegate.h"

namespace brave_ads {

struct ConfirmationInfo;

class RedeemConfirmationFactory final {
 public:
  static void BuildAndRedeemConfirmation(
      base::WeakPtr<RedeemConfirmationDelegate> delegate,
      const ConfirmationInfo& confirmation);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_CONFIRMATION_REDEEM_CONFIRMATION_FACTORY_H_
