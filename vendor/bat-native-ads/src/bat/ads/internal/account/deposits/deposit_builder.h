/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_DEPOSITS_DEPOSIT_BUILDER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_DEPOSITS_DEPOSIT_BUILDER_H_

#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

struct DepositInfo;

DepositInfo BuildDeposit(const mojom::SearchResultAdPtr& ad_mojom);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_DEPOSITS_DEPOSIT_BUILDER_H_
