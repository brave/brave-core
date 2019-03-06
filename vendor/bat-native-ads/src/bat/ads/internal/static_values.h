/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_STATIC_VALUES_H_
#define BAT_ADS_INTERNAL_STATIC_VALUES_H_

#include <stdint.h>

#include "base/time/time.h"

namespace ads {

#define STAGING_SERVER "https://ads-serve.bravesoftware.com"
#define PRODUCTION_SERVER "https://ads-serve.brave.com"

#define CATALOG_PATH "/v1/catalog"

static const int kIdleThresholdInSeconds = 15;

static const uint64_t kMaximumEntriesInPageScoreHistory = 5;
static const uint64_t kMaximumEntriesInAdsShownHistory = 99;

static const uint64_t kDebugOneHourInSeconds = 25;

static char kEasterEggUrl[] = "iab.com";
static const uint64_t kNextEasterEggStartsInSeconds = 30;

static const uint64_t kSustainAdInteractionAfterSeconds = 10;

static const uint64_t kDeliverNotificationsAfterSeconds =
    5 * base::Time::kSecondsPerMinute;

static const uint64_t kDefaultCatalogPing = 2 * base::Time::kSecondsPerHour;

static char kDefaultLanguageCode[] = "en";
static char kDefaultCountryCode[] = "US";

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_STATIC_VALUES_H_
