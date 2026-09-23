/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/deposits/deposit_builder.h"

#include "base/check.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposit_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

DepositInfo FromMojomBuildDeposit(
    const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad) {
  CHECK(mojom_creative_ad);

  return DepositInfo{
      .creative_instance_id = mojom_creative_ad->creative_instance_id,
      .value = mojom_creative_ad->value,
      .expire_at = base::Time::Now() + base::Hours(1),
  };
}

}  // namespace brave_ads
