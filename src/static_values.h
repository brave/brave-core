/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#define ADS_STAGING_SERVER "https://ads-serve-staging.brave.com/v1/catalog"
#define ADS_PRODUCTION_SERVER "https://ads-serve.mercury.basicattentiontoken.org/v1/catalog"

namespace ads {

static const uint64_t kMaximumEntriesInPageScoreHistory = 5;
static const uint64_t kMaximumEntriesInAdsShownHistory = 99;
static const uint64_t kMillisecondsInASecond = 1000;

static const uint64_t kOneHourInSeconds = 60 * 60;

static const uint64_t kDefaultAdsPerDay = 20;
static const uint64_t kDefaultAdsPerHour = 6;

}  // namespace ads
