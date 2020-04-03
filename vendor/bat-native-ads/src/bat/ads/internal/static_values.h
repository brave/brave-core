/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_STATIC_VALUES_H_
#define BAT_ADS_INTERNAL_STATIC_VALUES_H_

#include <stdint.h>
#include <string>
#include <map>
#include <vector>

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

const std::map<int, std::vector<std::string>> kSupportedRegionsSchemas = {
  // Append newly supported regions with a new schema version and update
  // |kSupportedRegionsSchemaVersionNumber| to match the new version
  //
  //   Format: { schema_version : { region... } }
  {
    1, {
      "US",   // United States of America
      "CA",   // Canada
      "GB",   // United Kingdom (Great Britain and Northern Ireland)
      "DE",   // Germany
      "FR"    // France
    }
  },
  {
    2, {
      "AU",   // Australia
      "NZ",   // New Zealand
      "IE"    // Ireland
    }
  },
  {
    3, {
      "AR",   // Argentina
      "AT",   // Austria
      "BR",   // Brazil
      "CH",   // Switzerland
      "CL",   // Chile
      "CO",   // Colombia
      "DK",   // Denmark
      "EC",   // Ecuador
      "IL",   // Israel
      "IN",   // India
      "IT",   // Italy
      "JP",   // Japan
      "KR",   // Korea
      "MX",   // Mexico
      "NL",   // Netherlands
      "PE",   // Peru
      "PH",   // Philippines
      "PL",   // Poland
      "SE",   // Sweden
      "SG",   // Singapore
      "VE",   // Venezuela
      "ZA"    // South Africa
    }
  },
  {
    4, {
      "KY"    // Cayman Islands
    }
  },
  {
    5, {
      "AE",   // United Arab Emirates
      "AL",   // Albania
      "AZ",   // Azerbaijan
      "BD",   // Bangladesh
      "BE",   // Belgium
      "BG",   // Bulgaria
      "CN",   // China
      "CZ",   // Czechia
      "DZ",   // Algeria
      "EG",   // Egypt
      "ES",   // Spain
      "FI",   // Finland
      "GR",   // Greece
      "HK",   // Hong Kong
      "HR",   // Croatia
      "HU",   // Hungary
      "ID",   // Indonesia
      "IQ",   // Iraq
      "KH",   // Cambodia
      "LT",   // Lithuania
      "MA",   // Morocco
      "MY",   // Malaysia
      "NG",   // Nigeria
      "NO",   // Norway
      "PK",   // Pakistan
      "PT",   // Portugal
      "RO",   // Romania
      "RS",   // Serbia
      "RU",   // Russia
      "SA",   // Saudi Arabia
      "SI",   // Slovenia
      "SK",   // Slovakia
      "TH",   // Thailand
      "TR",   // Turkey
      "TW",   // Taiwan
      "UA",   // Ukraine
      "VN"    // Vietnam
    }
  },
  {
    6, {
      "AF",   // Afghanistan
      "AS",   // American Samoa
      "AD",   // Andorra
      "AO",   // Angola
      "AI",   // Anguilla
      "AQ",   // Antarctica
      "AG",   // Antigua and Barbuda
      "AM",   // Armenia
      "AW",   // Aruba
      "BS",   // Bahamas
      "BH",   // Bahrain
      "BB",   // Barbados
      "BY",   // Belarus
      "BZ",   // Belize
      "BJ",   // Benin
      "BM",   // Bermuda
      "BT",   // Bhutan
      "BO",   // Bolivia
      "BQ",   // Bonaire
      "BA",   // Bosnia and Herzegovina
      "BW",   // Botswana
      "BV",   // Bouvet Island
      "IO",   // British Indian Ocean Territory
      "BN",   // Brunei Darussalam
      "BF",   // Burkina Faso
      "BI",   // Burundi
      "CM",   // Cameroon
      "CV",   // Cape Verde
      "TD",   // Chad
      "CX",   // Christmas Island
      "CC",   // Cocos (Keeling) Islands
      "KM",   // Comoros
      "CG",   // Republic of the Congo
      "CD",   // Democratic Republic of the Congo
      "CK",   // Cook Islands
      "CR",   // Costa Rica
      "CW",   // Curacao
      "CY",   // Cyprus
      "CI",   // Cote d'Ivoire
      "DJ",   // Djibouti
      "DM",   // Dominica
      "DO",   // Dominican Republic
      "SV",   // El Salvador
      "GQ",   // Equatorial Guinea
      "ER",   // Eritrea
      "EE",   // Estonia
      "ET",   // Ethiopia
      "FK",   // Falkland Islands
      "FO",   // Faroe Islands
      "FJ",   // Fiji
      "GF",   // French Guiana
      "PF",   // French Polynesia
      "TF",   // French and Antarctic Lands
      "GA",   // Gabon
      "GM",   // Gambia
      "GE",   // Georgia
      "GH",   // Ghana
      "GI",   // Gibraltar
      "GL",   // Greenland
      "GD",   // Grenada
      "GP",   // Guadeloupe
      "GU",   // Guam
      "GT",   // Guatemala
      "GG",   // Guernsey
      "GN",   // Guinea
      "GW",   // Guinea-Bissau
      "GY",   // Guyana
      "HT",   // Haiti
      "HM",   // Heard Island and McDonald Islands
      "VA",   // Vatican City
      "HN",   // Honduras
      "IS",   // Iceland
      "IM",   // Isle of Man
      "JM",   // Jamaica
      "JE",   // Jersey
      "JO",   // Jordan
      "KZ",   // Kazakhstan
      "KE",   // Kenya
      "KI",   // Kiribati
      "KW",   // Kuwait
      "KG",   // Kyrgyzstan
      "LA",   // Lao People's Democratic Republic
      "LV",   // Latvia
      "LB",   // Lebanon
      "LS",   // Lesotho
      "LR",   // Liberia
      "LI",   // Liechtenstein
      "LU",   // Luxembourg
      "MO",   // Macao
      "MK",   // Macedonia
      "MG",   // Madagascar
      "MW",   // Malawi
      "MV",   // Maldives
      "ML",   // Mali
      "MT",   // Malta
      "MH",   // Marshall Islands
      "MQ",   // Martinique
      "MR",   // Mauritania
      "MU",   // Mauritius
      "YT",   // Mayotte
      "FM",   // Micronesia, Federated States of
      "MD",   // Moldova
      "MC",   // Monaco
      "MN",   // Mongolia
      "ME",   // Montenegro
      "MS",   // Montserrat
      "MZ",   // Mozambique
      "MM",   // Myanmar
      "NA",   // Namibia
      "NR",   // Nauru
      "NP",   // Nepal
      "NC",   // New Caledonia
      "NI",   // Nicaragua
      "NE",   // Niger
      "NU",   // Niue
      "NF",   // Norfolk Island
      "MP",   // Northern Mariana Islands
      "NO",   // Norway
      "OM",   // Oman
      "PW",   // Palau
      "PS",   // Palestine
      "PA",   // Panama
      "PG",   // Papua New Guinea
      "PY",   // Paraguay
      "PN",   // Pitcairn
      "PR",   // Puerto Rico
      "QA",   // Qatar
      "RW",   // Rwanda
      "RE",   // Reunion
      "BL",   // Saint Barthelemy
      "SH",   // Saint Helena
      "KN",   // Saint Kitts and Nevis
      "LC",   // Saint Lucia
      "MF",   // Saint Martin
      "PM",   // Saint Pierre and Miquelon
      "VC",   // Saint Vincent and the Grenadines
      "WS",   // Samoa
      "SM",   // San Marino
      "ST",   // São Tomé and Príncipe
      "SN",   // Senegal
      "SC",   // Seychelles
      "SL",   // Sierra Leone
      "SX",   // Sint Maarten
      "SB",   // Solomon Islands
      "GS",   // South Georgia and the South Sandwich Islands
      "SS",   // South Sudan
      "LK",   // Sri Lanka
      "SR",   // Suriname
      "SJ",   // Svalbard and Jan Mayen
      "SZ",   // Swaziland
      "TJ",   // Tajikistan
      "TZ",   // United Republic of Tanzania
      "TL",   // Timor-Leste
      "TG",   // Togo
      "TK",   // Tokelau
      "TO",   // Tonga
      "TT",   // Trinidad and Tobago
      "TN",   // Tunisia
      "TM",   // Turkmenistan
      "TC",   // Turks and Caicos Islands
      "TV",   // Tuvalu
      "UG",   // Uganda
      "UM",   // United States Minor Outlying Islands
      "UY",   // Uruguay
      "UZ",   // Uzbekistan
      "VU",   // Vanuatu
      "VG",   // British Virgin Islands
      "VI",   // US Virgin Islands
      "WF",   // Wallis and Futuna
      "EH",   // Western Sahara
      "YE",   // Yemen
      "ZM",   // Zambia
      "ZW"    // Zimbabwe
    }
  },
  {
    7, {
      "101",  // World
      "150"   // Europe
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
