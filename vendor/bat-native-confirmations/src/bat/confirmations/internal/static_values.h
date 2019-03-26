/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_STATIC_VALUES_H_
#define BAT_CONFIRMATIONS_INTERNAL_STATIC_VALUES_H_

#include <stdint.h>

#include "base/time/time.h"

namespace confirmations {

#define BAT_ADS_STAGING_SERVER "https://ads-serve.bravesoftware.com"
#define BAT_ADS_PRODUCTION_SERVER "https://ads-serve.brave.com"

static const int kMinimumUnblindedTokens = 20;
static const int kMaximumUnblindedTokens = 50;

static const uint64_t kRetryGettingRefillSignedTokensAfterSeconds = 15;

static const uint64_t kNextTokenRedemptionAfterSeconds =
    base::Time::kMicrosecondsPerWeek / base::Time::kMicrosecondsPerSecond;

static const uint64_t kDebugNextTokenRedemptionAfterSeconds =
    25 * base::Time::kSecondsPerMinute;

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_STATIC_VALUES_H_
