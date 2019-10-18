/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_STATIC_VALUES_H_
#define BAT_CONFIRMATIONS_INTERNAL_STATIC_VALUES_H_

#include <stdint.h>

#include "base/time/time.h"

namespace confirmations {

#define BAT_ADS_PRODUCTION_SERVER "https://ads-serve.brave.com"
#define BAT_ADS_STAGING_SERVER "https://ads-serve.bravesoftware.com"
#define BAT_ADS_DEVELOPMENT_SERVER "https://ads-serve.brave.software.com"

#define BAT_LEDGER_PRODUCTION_SERVER "https://ledger.mercury.basicattentiontoken.org"  // NOLINT
#define BAT_LEDGER_STAGING_SERVER "https://ledger-staging.mercury.basicattentiontoken.org"  // NOLINT
#define BAT_LEDGER_DEVELOPMENT_SERVER "https://ledger.rewards.brave.software.org"  // NOLINT

static const int kNextPaymentDay = 5;

static const int kMinimumUnblindedTokens = 20;
static const int kMaximumUnblindedTokens = 50;

static const uint64_t kRetryGettingRefillSignedTokensAfterSeconds = 15;

static const uint64_t kNextTokenRedemptionAfterSeconds =
    24 * base::Time::kSecondsPerHour;
static const uint64_t kDebugNextTokenRedemptionAfterSeconds =
    25 * base::Time::kSecondsPerMinute;

static const uint64_t kRetryFailedConfirmationsAfterSeconds =
    5 * base::Time::kSecondsPerMinute;

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_STATIC_VALUES_H_
