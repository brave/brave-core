/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_STATIC_VALUES_H_
#define BAT_ADS_INTERNAL_STATIC_VALUES_H_

#include <stdint.h>
#include <string>
#include <map>

#include "base/time/time.h"

namespace ads {

#define STAGING_SERVER "https://ads-serve.bravesoftware.com"
#define PRODUCTION_SERVER "https://ads-serve.brave.com"

#define CATALOG_PATH "/v1/catalog"

static const int kIdleThresholdInSeconds = 15;

static const uint64_t kMaximumEntriesInPageScoreHistory = 5;
static const uint64_t kMaximumEntriesInAdsShownHistory = 99;

static const uint64_t kDebugOneHourInSeconds = 25;

static char kEasterEggUrl[] = "https://iab.com";
static const uint64_t kNextEasterEggStartsInSeconds = 30;

static char kShoppingStateUrl[] = "https://amazon.com";

static const uint64_t kSustainAdInteractionAfterSeconds = 10;

static const uint64_t kDefaultCatalogPing = 2 * base::Time::kSecondsPerHour;
static const uint64_t kDebugCatalogPing = 15 * base::Time::kSecondsPerMinute;

static char kDefaultLanguageCode[] = "en";
static char kDefaultCountryCode[] = "US";

static const std::map<std::string, bool> kSupportedRegions = {
  // {{region, targeted}}

  { "US", true  },  // United States of America
  { "CA", true  },  // Canada
  { "GB", true  },  // United Kingdom (Great Britain and Northern Ireland)
  { "DE", true  },  // Germany
  { "FR", true  },  // France
  { "AU", true  },  // Australia
  { "NZ", true  },  // New Zealand
  { "IE", true  },  // Ireland
  { "AR", false },  // Argentina
  { "AT", false },  // Austria
  { "BR", false },  // Brazil
  { "CH", false },  // Switzerland
  { "CL", false },  // Chile
  { "CO", false },  // Colombia
  { "DK", false },  // Denmark
  { "EC", false },  // Ecuador
  { "IL", false },  // Israel
  { "IN", false },  // India
  { "IT", false },  // Italy
  { "KR", false },  // Korea
  { "MX", false },  // Mexico
  { "NL", false },  // Netherlands
  { "PE", false },  // Peru
  { "PH", false },  // Philippines
  { "PL", false },  // Poland
  { "SE", false },  // Sweden
  { "SG", false },  // Singapore
  { "VE", false },  // Venezuela
  { "ZA", false }   // South Africa
};

static char kUntargetedPageClassification[] = "untargeted";

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_STATIC_VALUES_H_
