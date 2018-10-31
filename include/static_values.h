/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#define ADS_STAGING_SERVER "https://ads-serve-staging.brave.com/v1/catalog"
#define ADS_PRODUCTION_SERVER "https://ads-serve.mercury.basicattentiontoken.org/v1/catalog"

namespace rewards_ads {

static const uint64_t _maximum_entries_in_page_score_history = 5;
static const uint64_t _maximum_entries_in_ads_shown_history = 99;
static const uint64_t _milliseconds_in_a_second = 1000;

static const uint64_t _one_hour_in_seconds = 60 * 60;

static const uint64_t _default_ads_per_day = 20;
static const uint64_t _default_ads_per_hour = 6;

}  // namespace rewards_ads
