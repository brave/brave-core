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

#define CATALOG_PATH "/v2/catalog"

const int kIdleThresholdInSeconds = 15;

const uint64_t kMaximumEntriesInPageScoreHistory = 5;
const int kWinningCategoryCountForServingAds = 3;

// Maximum entries based upon 7 days of history, 20 ads per day and 4
// confirmation types
const uint64_t kMaximumEntriesInAdsShownHistory = 7 * (20 * 4);

const uint64_t kMaximumEntriesPerSegmentInPurchaseIntentSignalHistory = 100;

const uint64_t kDebugOneHourInSeconds = 25;

const char kEasterEggUrl[] = "https://iab.com";
const uint64_t kNextEasterEggStartsInSeconds = 30;

const char kShoppingStateUrl[] = "https://amazon.com";

const uint64_t kSustainAdNotificationInteractionAfterSeconds = 10;

const uint64_t kDefaultCatalogPing = 2 * base::Time::kSecondsPerHour;
const uint64_t kDebugCatalogPing = 15 * base::Time::kSecondsPerMinute;

const int kDebugAdConversionFrequency = 10 * base::Time::kSecondsPerMinute;
const int kAdConversionFrequency =
    base::Time::kHoursPerDay * base::Time::kSecondsPerHour;
const int kExpiredAdConversionFrequency = 5 * base::Time::kSecondsPerMinute;

const char kDefaultLanguage[] = "en";
const char kDefaultRegion[] = "US";
const char kDefaultUserModelLanguage[] = "en";

const uint16_t kPurchaseIntentSignalLevel = 1;
const uint16_t kPurchaseIntentClassificationThreshold = 10;
const uint64_t kPurchaseIntentSignalDecayTimeWindow =
    base::Time::kSecondsPerHour * base::Time::kHoursPerDay * 7;
const uint16_t kPurchaseIntentMaxSegments = 3;

const int kDoNotDisturbFromHour = 21;  // 9pm
const int kDoNotDisturbToHour = 6;     // 6am

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
      { "RU", false },  // Russia
      { "SA", false },  // Saudi Arabia
      { "SI", false },  // Slovenia
      { "SK", false },  // Slovakia
      { "TH", false },  // Thailand
      { "TR", false },  // Turkey
      { "TW", false },  // Taiwan
      { "UA", false },  // Ukraine
      { "VN", false }   // Vietnam
    }
  },
  {
    6, {
      { "AF", false },  // Afghanistan
      { "AS", false },  // American Samoa
      { "AD", false },  // Andorra
      { "AO", false },  // Angola
      { "AI", false },  // Anguilla
      { "AQ", false },  // Antarctica
      { "AG", false },  // Antigua and Barbuda
      { "AM", false },  // Armenia
      { "AW", false },  // Aruba
      { "BS", false },  // Bahamas
      { "BH", false },  // Bahrain
      { "BB", false },  // Barbados
      { "BY", false },  // Belarus
      { "BZ", false },  // Belize
      { "BJ", false },  // Benin
      { "BM", false },  // Bermuda
      { "BT", false },  // Bhutan
      { "BO", false },  // Bolivia
      { "BQ", false },  // Bonaire
      { "BA", false },  // Bosnia and Herzegovina
      { "BW", false },  // Botswana
      { "BV", false },  // Bouvet Island
      { "IO", false },  // British Indian Ocean Territory
      { "BN", false },  // Brunei Darussalam
      { "BF", false },  // Burkina Faso
      { "BI", false },  // Burundi
      { "CM", false },  // Cameroon
      { "CV", false },  // Cape Verde
      { "TD", false },  // Chad
      { "CX", false },  // Christmas Island
      { "CC", false },  // Cocos (Keeling) Islands
      { "KM", false },  // Comoros
      { "CG", false },  // Republic of the Congo
      { "CD", false },  // Democratic Republic of the Congo
      { "CK", false },  // Cook Islands
      { "CR", false },  // Costa Rica
      { "CW", false },  // Curacao
      { "CY", false },  // Cyprus
      { "CI", false },  // Cote d'Ivoire
      { "DJ", false },  // Djibouti
      { "DM", false },  // Dominica
      { "DO", false },  // Dominican Republic
      { "SV", false },  // El Salvador
      { "GQ", false },  // Equatorial Guinea
      { "ER", false },  // Eritrea
      { "EE", false },  // Estonia
      { "ET", false },  // Ethiopia
      { "FK", false },  // Falkland Islands
      { "FO", false },  // Faroe Islands
      { "FJ", false },  // Fiji
      { "GF", false },  // French Guiana
      { "PF", false },  // French Polynesia
      { "TF", false },  // French and Antarctic Lands
      { "GA", false },  // Gabon
      { "GM", false },  // Gambia
      { "GE", false },  // Georgia
      { "GH", false },  // Ghana
      { "GI", false },  // Gibraltar
      { "GL", false },  // Greenland
      { "GD", false },  // Grenada
      { "GP", false },  // Guadeloupe
      { "GU", false },  // Guam
      { "GT", false },  // Guatemala
      { "GG", false },  // Guernsey
      { "GN", false },  // Guinea
      { "GW", false },  // Guinea-Bissau
      { "GY", false },  // Guyana
      { "HT", false },  // Haiti
      { "HM", false },  // Heard Island and McDonald Islands
      { "VA", false },  // Vatican City
      { "HN", false },  // Honduras
      { "IS", false },  // Iceland
      { "IM", false },  // Isle of Man
      { "JM", false },  // Jamaica
      { "JE", false },  // Jersey
      { "JO", false },  // Jordan
      { "KZ", false },  // Kazakhstan
      { "KE", false },  // Kenya
      { "KI", false },  // Kiribati
      { "KW", false },  // Kuwait
      { "KG", false },  // Kyrgyzstan
      { "LA", false },  // Lao People's Democratic Republic
      { "LV", false },  // Latvia
      { "LB", false },  // Lebanon
      { "LS", false },  // Lesotho
      { "LR", false },  // Liberia
      { "LI", false },  // Liechtenstein
      { "LU", false },  // Luxembourg
      { "MO", false },  // Macao
      { "MK", false },  // Macedonia
      { "MG", false },  // Madagascar
      { "MW", false },  // Malawi
      { "MV", false },  // Maldives
      { "ML", false },  // Mali
      { "MT", false },  // Malta
      { "MH", false },  // Marshall Islands
      { "MQ", false },  // Martinique
      { "MR", false },  // Mauritania
      { "MU", false },  // Mauritius
      { "YT", false },  // Mayotte
      { "FM", false },  // Micronesia, Federated States of
      { "MD", false },  // Moldova
      { "MC", false },  // Monaco
      { "MN", false },  // Mongolia
      { "ME", false },  // Montenegro
      { "MS", false },  // Montserrat
      { "MZ", false },  // Mozambique
      { "MM", false },  // Myanmar
      { "NA", false },  // Namibia
      { "NR", false },  // Nauru
      { "NP", false },  // Nepal
      { "NC", false },  // New Caledonia
      { "NI", false },  // Nicaragua
      { "NE", false },  // Niger
      { "NU", false },  // Niue
      { "NF", false },  // Norfolk Island
      { "MP", false },  // Northern Mariana Islands
      { "NO", false },  // Norway
      { "OM", false },  // Oman
      { "PW", false },  // Palau
      { "PS", false },  // Palestine
      { "PA", false },  // Panama
      { "PG", false },  // Papua New Guinea
      { "PY", false },  // Paraguay
      { "PN", false },  // Pitcairn
      { "PR", false },  // Puerto Rico
      { "QA", false },  // Qatar
      { "RW", false },  // Rwanda
      { "RE", false },  // Reunion
      { "BL", false },  // Saint Barthelemy
      { "SH", false },  // Saint Helena
      { "KN", false },  // Saint Kitts and Nevis
      { "LC", false },  // Saint Lucia
      { "MF", false },  // Saint Martin
      { "PM", false },  // Saint Pierre and Miquelon
      { "VC", false },  // Saint Vincent and the Grenadines
      { "WS", false },  // Samoa
      { "SM", false },  // San Marino
      { "ST", false },  // São Tomé and Príncipe
      { "SN", false },  // Senegal
      { "SC", false },  // Seychelles
      { "SL", false },  // Sierra Leone
      { "SX", false },  // Sint Maarten
      { "SB", false },  // Solomon Islands
      { "GS", false },  // South Georgia and the South Sandwich Islands
      { "SS", false },  // South Sudan
      { "LK", false },  // Sri Lanka
      { "SR", false },  // Suriname
      { "SJ", false },  // Svalbard and Jan Mayen
      { "SZ", false },  // Swaziland
      { "TJ", false },  // Tajikistan
      { "TZ", false },  // United Republic of Tanzania
      { "TL", false },  // Timor-Leste
      { "TG", false },  // Togo
      { "TK", false },  // Tokelau
      { "TO", false },  // Tonga
      { "TT", false },  // Trinidad and Tobago
      { "TN", false },  // Tunisia
      { "TM", false },  // Turkmenistan
      { "TC", false },  // Turks and Caicos Islands
      { "TV", false },  // Tuvalu
      { "UG", false },  // Uganda
      { "UM", false },  // United States Minor Outlying Islands
      { "UY", false },  // Uruguay
      { "UZ", false },  // Uzbekistan
      { "VU", false },  // Vanuatu
      { "VG", false },  // British Virgin Islands
      { "VI", false },  // US Virgin Islands
      { "WF", false },  // Wallis and Futuna
      { "EH", false },  // Western Sahara
      { "YE", false },  // Yemen
      { "ZM", false },  // Zambia
      { "ZW", false }   // Zimbabwe
    }
  }

  // IMPORTANT: When adding new schema versions |newly_supported_locale_| must
  // be updated in |BraveRewardsBrowserTest| to reflect a locale from the latest
  // schema version in "bat-native-ads/src/bat/ads/internal/static_values.h"
};

const char kUntargetedPageClassification[] = "untargeted";

#if defined(OS_ANDROID)
const int kMaximumAdNotifications = 3;
#else
const int kMaximumAdNotifications = 0;  // No limit
#endif

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_STATIC_VALUES_H_
