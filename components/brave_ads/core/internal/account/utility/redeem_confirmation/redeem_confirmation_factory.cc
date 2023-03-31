/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/redeem_confirmation_factory.h"

#include "brave/components/brave_ads/core/internal/account/account_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/redeem_opted_in_confirmation.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/redeem_opted_out_confirmation.h"

namespace brave_ads {

RedeemConfirmationInterface* RedeemConfirmationFactory::Build() {
  if (!ShouldRewardUser()) {
    return RedeemOptedOutConfirmation::Create();
  }

  return RedeemOptedInConfirmation::Create();
}

}  // namespace brave_ads
