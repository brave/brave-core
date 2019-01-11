/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_STATIC_VALUES_H_
#define BAT_ADS_STATIC_VALUES_H_

#include <string>

namespace ads {

#define STAGING_SERVER "https://ads-serve.bravesoftware.com"
#define PRODUCTION_SERVER "https://ads-serve.brave.com"

#define CATALOG_PATH "/v1/catalog"

static const int kIdleThresholdInSeconds = 15;

static const uint64_t kMaximumEntriesInPageScoreHistory = 5;
static const uint64_t kMaximumEntriesInAdsShownHistory = 99;

static const uint64_t kMillisecondsInASecond = 1000;

static const uint64_t kOneMinuteInSeconds = 60;
static const uint64_t kOneHourInSeconds = 60 * kOneMinuteInSeconds;
static const uint64_t kDebugOneHourInSeconds = 25;
static const uint64_t kOneDayInSeconds = 24 * kOneHourInSeconds;

static char kEasterEggUrl[] = "www.iab.com";
static const uint64_t kNextEasterEggStartsInSeconds = 30;

static const uint64_t kSustainAdInteractionAfterSeconds = 10;

static const uint64_t kDeliverNotificationsAfterSeconds =
  5 * kOneMinuteInSeconds;

static const uint64_t kDefaultCatalogPing = 2 * kOneHourInSeconds;

static char kDefaultLanguageCode[] = "en";
static char kDefaultCountryCode[] = "US";

}  // namespace ads

#endif  // BAT_ADS_STATIC_VALUES_H_
