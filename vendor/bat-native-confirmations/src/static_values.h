/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_STATIC_VALUES_H_
#define BAT_CONFIRMATIONS_STATIC_VALUES_H_

namespace confirmations {

#define BAT_ADS_STAGING_SERVER "https://ads-serve.bravesoftware.com"
#define BAT_ADS_PRODUCTION_SERVER "https://ads-serve.brave.com"

static const int kMinimumUnblindedTokens = 20;
static const int kMaximumUnblindedTokens = 50;

static const uint64_t kOneMinuteInSeconds = 60;
static const uint64_t kOneHourInSeconds = 60 * kOneMinuteInSeconds;
static const uint64_t kOneDayInSeconds = 24 * kOneHourInSeconds;

static const uint64_t kRetryGettingRefillSignedTokensAfterSeconds = 15;
static const uint64_t kPayoutAfterSeconds = kOneDayInSeconds;

}  // namespace confirmations

#endif  // BAT_ADS_STATIC_VALUES_H_
