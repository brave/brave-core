/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/deposits/deposit_builder.h"

#include "base/time/time.h"
#include "bat/ads/internal/account/deposits/deposit_info.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

DepositInfo BuildDeposit(const mojom::SearchResultAdInfoPtr& ad_mojom) {
  DepositInfo deposit;

  deposit.creative_instance_id = ad_mojom->creative_instance_id;
  deposit.value = ad_mojom->value;
  deposit.expire_at = base::Time::Now() + base::Hours(1);

  return deposit;
}

}  // namespace ads
