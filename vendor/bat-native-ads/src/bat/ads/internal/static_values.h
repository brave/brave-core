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

#define PRODUCTION_SERVER "https://ads-serve.brave.com"
#define STAGING_SERVER "https://ads-serve.bravesoftware.com"
#define DEVELOPMENT_SERVER "https://ads-serve.brave.software"

const int kCatalogVersion = 3;

const int kIdleThresholdInSeconds = 15;

const uint64_t kMaximumEntriesInPageScoreHistory = 5;
const int kWinningCategoryCountForServingAds = 3;

// Maximum entries based upon 7 days of history, 20 ads per day and 4
// confirmation types
const uint64_t kMaximumEntriesInAdsShownHistory = 7 * (20 * 4);

const uint64_t kDebugOneHourInSeconds = 25;

const char kEasterEggUrl[] = "https://iab.com";
const uint64_t kNextEasterEggStartsInSeconds = 30;

const char kShoppingStateUrl[] = "https://amazon.com";

const uint64_t kSustainAdNotificationInteractionAfterSeconds = 10;
const uint64_t kSustainPublisherAdInteractionAfterSeconds = 10;

const uint64_t kDefaultCatalogPing = 2 * base::Time::kSecondsPerHour;
const uint64_t kDebugCatalogPing = 15 * base::Time::kSecondsPerMinute;

const int kDebugAdConversionFrequency = 10 * base::Time::kSecondsPerMinute;
const int kAdConversionFrequency =
    base::Time::kHoursPerDay * base::Time::kSecondsPerHour;
const int kExpiredAdConversionFrequency = 5 * base::Time::kSecondsPerMinute;

const char kDefaultLanguage[] = "en";
const char kDefaultRegion[] = "US";
const char kDefaultUserModelLanguage[] = "en";

static const int kDoNotDisturbFromHour = 21;  // 9pm
static const int kDoNotDisturbToHour = 6;     // 6am

const std::map<int, std::map<std::string, bool>> kSupportedRegionsSchemas = {
  // Append newly supported regions with a new schema version and update
  // |kSupportedRegionsSchemaVersionNumber| to match the new version
  //
  //   Format: { schema_version : {{region, targeted}} }
  //
  // If |targeted| is set to |true| web pages are classified using the
  // "bat-native-usermodel", otherwise untargeted ads are delivered
  {
    1, {
      { "US", true  },  // United States of America
      { "CA", true  },  // Canada
      { "GB", true  },  // United Kingdom (Great Britain and Northern Ireland)
      { "DE", true  },  // Germany
      { "FR", true  }   // France
    }
  },
  {
    2, {
      { "AU", true  },  // Australia
      { "NZ", true  },  // New Zealand
      { "IE", true  }   // Ireland
    }
  },
  {
    3, {
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
      { "JP", false },  // Japan
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
    }
  },
  {
    4, {
      { "KY", true  }   // Cayman Islands
    }
  },
  {
    5, {
      { "AE", false },  // United Arab Emirates
      { "AL", false },  // Albania
      { "AZ", false },  // Azerbaijan
      { "BD", false },  // Bangladesh
      { "BE", false },  // Belgium
      { "BG", false },  // Bulgaria
      { "CN", false },  // China
      { "CZ", false },  // Czechia
      { "DZ", false },  // Algeria
      { "EG", false },  // Egypt
      { "ES", false },  // Spain
      { "FI", false },  // Finland
      { "GR", false },  // Greece
      { "HK", false },  // Hong Kong
      { "HR", false },  // Croatia
      { "HU", false },  // Hungary
      { "ID", false },  // Indonesia
      { "IQ", false },  // Iraq
      { "KH", false },  // Cambodia
      { "LT", false },  // Lithuania
      { "MA", false },  // Morocco
      { "MY", false },  // Malaysia
      { "NG", false },  // Nigeria
      { "NO", false },  // Norway
      { "PK", false },  // Pakistan
      { "PT", false },  // Portugal
      { "RO", false },  // Romania
      { "RS", false },  // Serbia
      { "RU", false },  // Russian Federation
      { "SA", false },  // Saudi Arabia
      { "SI", false },  // Slovenia
      { "SK", false },  // Slovakia
      { "TH", false },  // Thailand
      { "TR", false },  // Turkey
      { "TW", false },  // Taiwan
      { "UA", false },  // Ukraine
      { "VN", false }   // Vietnam
    }
  }

  // IMPORTANT: When adding new schema versions |newly_supported_locale_| must
  // be updated in |BraveRewardsBrowserTest| to reflect a locale from the latest
  // schema version in "bat-native-ads/src/bat/ads/internal/static_values.h"
};

const char kUntargetedPageClassification[] = "untargeted";

#if defined(OS_ANDROID)
const int kMaximumAdNotifications = 3;
#endif

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_STATIC_VALUES_H_
