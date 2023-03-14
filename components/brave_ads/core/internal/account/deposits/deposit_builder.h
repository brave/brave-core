/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_DEPOSITS_DEPOSIT_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_DEPOSITS_DEPOSIT_BUILDER_H_

#include "brave/components/brave_ads/common/interfaces/ads.mojom-forward.h"

namespace ads {

struct DepositInfo;

DepositInfo BuildDeposit(const mojom::SearchResultAdInfoPtr& ad_mojom);

}  // namespace ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_DEPOSITS_DEPOSIT_BUILDER_H_
