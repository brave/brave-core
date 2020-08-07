/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_FREQUENCY_CAPPING_FREQUENCY_CAPPING_UNITTEST_UTIL_H_
#define BAT_ADS_INTERNAL_FREQUENCY_CAPPING_FREQUENCY_CAPPING_UNITTEST_UTIL_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "bat/ads/ad_history.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"

namespace ads {

AdHistory GenerateAdHistory(
    const CreativeAdInfo& ad,
    const ConfirmationType& confirmation_type);

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_FREQUENCY_CAPPING_FREQUENCY_CAPPING_UNITTEST_UTIL_H_  // NOLINT
