/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_STATIC_VALUES_H_
#define BAT_CONFIRMATIONS_INTERNAL_STATIC_VALUES_H_

#include <stdint.h>
#include <map>
#include <string>

#include "base/time/time.h"

namespace confirmations {

#define BAT_ADS_PRODUCTION_SERVER "https://ads-serve.brave.com"
#define BAT_ADS_STAGING_SERVER "https://ads-serve.bravesoftware.com"
#define BAT_ADS_DEVELOPMENT_SERVER "https://ads-serve.brave.software"

#define PROMOTION_STAGING_SERVER "https://grant.rewards.bravesoftware.com"
#define PROMOTION_PRODUCTION_SERVER "https://grant.rewards.brave.com"
#define PROMOTION_DEVELOPMENT_SERVER "https://grant.rewards.brave.software"

const int kNextPaymentDay = 5;

const int kMinimumUnblindedTokens = 20;
const int kMaximumUnblindedTokens = 50;

const uint64_t kNextTokenRedemptionAfterSeconds =
    24 * base::Time::kSecondsPerHour;
const uint64_t kDebugNextTokenRedemptionAfterSeconds =
    25 * base::Time::kSecondsPerMinute;

const uint64_t kRetryRefillTokensAfterSeconds = 15;

const uint64_t kRetryPayoutTokensAfterSeconds =
    1 * base::Time::kSecondsPerMinute;

const uint64_t kRetryAdsRewardsAfterSeconds =
    1 * base::Time::kSecondsPerMinute;

const uint64_t kRetryFailedConfirmationsAfterSeconds =
    5 * base::Time::kSecondsPerMinute;

const std::map<std::string, bool> kLargeAnonymityCountryCodes = {
  { "US",  true  },  // United States of America
  { "CA",  true  },  // Canada
  { "GB",  true  },  // United Kingdom (Great Britain and Northern Ireland)
  { "DE",  true  },  // Germany
  { "FR",  true  },  // France
  { "AU",  true  },  // Australia
  { "NZ",  true  },  // New Zealand
  { "IE",  true  },  // Ireland
  { "AR",  true  },  // Argentina
  { "AT",  true  },  // Austria
  { "BR",  true  },  // Brazil
  { "CH",  true  },  // Switzerland
  { "CL",  true  },  // Chile
  { "CO",  true  },  // Colombia
  { "DK",  true  },  // Denmark
  { "EC",  true  },  // Ecuador
  { "IL",  true  },  // Israel
  { "IN",  true  },  // India
  { "IT",  true  },  // Italy
  { "JP",  true  },  // Japan
  { "KR",  true  },  // Korea
  { "MX",  true  },  // Mexico
  { "NL",  true  },  // Netherlands
  { "PE",  true  },  // Peru
  { "PH",  true  },  // Philippines
  { "PL",  true  },  // Poland
  { "SE",  true  },  // Sweden
  { "SG",  true  },  // Singapore
  { "VE",  true  },  // Venezuela
  { "ZA",  true  },  // South Africa
  { "KY",  false },  // Cayman Islands
  { "AE",  true  },  // United Arab Emirates
  { "AL",  true  },  // Albania
  { "AZ",  true  },  // Azerbaijan
  { "BD",  true  },  // Bangladesh
  { "BE",  true  },  // Belgium
  { "BG",  true  },  // Bulgaria
  { "CN",  true  },  // China
  { "CZ",  true  },  // Czechia
  { "DZ",  true  },  // Algeria
  { "EG",  true  },  // Egypt
  { "ES",  true  },  // Spain
  { "FI",  true  },  // Finland
  { "GR",  true  },  // Greece
  { "HK",  true  },  // Hong Kong
  { "HR",  true  },  // Croatia
  { "HU",  true  },  // Hungary
  { "ID",  true  },  // Indonesia
  { "IQ",  true  },  // Iraq
  { "KH",  true  },  // Cambodia
  { "LT",  true  },  // Lithuania
  { "MA",  true  },  // Morocco
  { "MY",  true  },  // Malaysia
  { "NG",  true  },  // Nigeria
  { "NO",  true  },  // Norway
  { "PK",  true  },  // Pakistan
  { "PT",  true  },  // Portugal
  { "RO",  true  },  // Romania
  { "RS",  true  },  // Serbia
  { "RU",  true  },  // Russian Federation
  { "SA",  true  },  // Saudi Arabia
  { "SI",  true  },  // Slovenia
  { "SK",  true  },  // Slovakia
  { "TH",  true  },  // Thailand
  { "TR",  true  },  // Turkey
  { "TW",  true  },  // Taiwan
  { "UA",  true  },  // Ukraine
  { "VN",  true  },  // Vietnam
  { "AF",  true  },  // Afghanistan
  { "AS",  false },  // American Samoa
  { "AD",  false },  // Andorra
  { "AO",  false },  // Angola
  { "AI",  false },  // Anguilla
  { "AQ",  false },  // Antarctica
  { "AG",  false },  // Antigua and Barbuda
  { "AM",  true  },  // Armenia
  { "AW",  false },  // Aruba
  { "BS",  true  },  // Bahamas
  { "BH",  true  },  // Bahrain
  { "BB",  true  },  // Barbados
  { "BY",  true  },  // Belarus
  { "BZ",  false },  // Belize
  { "BJ",  true  },  // Benin
  { "BM",  false },  // Bermuda
  { "BT",  false },  // Bhutan
  { "BO",  true  },  // Bolivia
  { "BQ",  false },  // Bonaire
  { "BA",  true  },  // Bosnia and Herzegovina
  { "BW",  true  },  // Botswana
  { "BV",  false },  // Bouvet Island
  { "IO",  false },  // British Indian Ocean Territory
  { "BN",  true  },  // Brunei Darussalam
  { "BF",  false },  // Burkina Faso
  { "BI",  false },  // Burundi
  { "CM",  true  },  // Cameroon
  { "CV",  false },  // Cape Verde
  { "TD",  false },  // Chad
  { "CX",  false },  // Christmas Island
  { "CC",  false },  // Cocos (Keeling) Islands
  { "KM",  false },  // Comoros
  { "CG",  false },  // Republic of the Congo
  { "CD",  true  },  // Democratic Republic of the Congo
  { "CK",  false },  // Cook Islands
  { "CR",  true  },  // Costa Rica
  { "CW",  false },  // Curacao
  { "CY",  true  },  // Cyprus
  { "CI",  true  },  // Cote d'Ivoire
  { "DJ",  false },  // Djibouti
  { "DM",  false },  // Dominica
  { "DO",  true  },  // Dominican Republic
  { "SV",  true  },  // El Salvador
  { "GQ",  false },  // Equatorial Guinea
  { "ER",  false },  // Eritrea
  { "EE",  true  },  // Estonia
  { "ET",  true  },  // Ethiopia
  { "FK",  false },  // Falkland Islands
  { "FO",  false },  // Faroe Islands
  { "FJ",  false },  // Fiji
  { "GF",  false },  // French Guiana
  { "PF",  false },  // French Polynesia
  { "TF",  false },  // French and Antarctic Lands
  { "GA",  false },  // Gabon
  { "GM",  false },  // Gambia
  { "GE",  true  },  // Georgia
  { "GH",  true  },  // Ghana
  { "GI",  false },  // Gibraltar
  { "GL",  false },  // Greenland
  { "GD",  false },  // Grenada
  { "GP",  false },  // Guadeloupe
  { "GU",  false },  // Guam
  { "GT",  true  },  // Guatemala
  { "GG",  false },  // Guernsey
  { "GN",  false },  // Guinea
  { "GW",  false },  // Guinea-Bissau
  { "GY",  false },  // Guyana
  { "HT",  false },  // Haiti
  { "HM",  false },  // Heard Island and McDonald Islands
  { "VA",  false },  // Vatican City
  { "HN",  true  },  // Honduras
  { "IS",  true  },  // Iceland
  { "IM",  false },  // Isle of Man
  { "JM",  true  },  // Jamaica
  { "JE",  false },  // Jersey
  { "JO",  true  },  // Jordan
  { "KZ",  true  },  // Kazakhstan
  { "KE",  true  },  // Kenya
  { "KI",  false },  // Kiribati
  { "KW",  true  },  // Kuwait
  { "KG",  true  },  // Kyrgyzstan
  { "LA",  false },  // Lao People's Democratic Republic
  { "LV",  true  },  // Latvia
  { "LB",  true  },  // Lebanon
  { "LS",  false },  // Lesotho
  { "LR",  false },  // Liberia
  { "LI",  false },  // Liechtenstein
  { "LU",  true  },  // Luxembourg
  { "MO",  false },  // Macao
  { "MK",  true  },  // Macedonia
  { "MG",  true  },  // Madagascar
  { "MW",  false },  // Malawi
  { "MV",  false },  // Maldives
  { "ML",  false },  // Mali
  { "MT",  true  },  // Malta
  { "MH",  false },  // Marshall Islands
  { "MQ",  false },  // Martinique
  { "MR",  false },  // Mauritania
  { "MU",  true  },  // Mauritius
  { "YT",  false },  // Mayotte
  { "FM",  false },  // Micronesia, Federated States of
  { "MD",  true  },  // Moldova
  { "MC",  false },  // Monaco
  { "MN",  true  },  // Mongolia
  { "ME",  true  },  // Montenegro
  { "MS",  false },  // Montserrat
  { "MZ",  false },  // Mozambique
  { "MM",  true  },  // Myanmar
  { "NA",  true  },  // Namibia
  { "NR",  false },  // Nauru
  { "NP",  true  },  // Nepal
  { "NC",  false },  // New Caledonia
  { "NI",  true  },  // Nicaragua
  { "NE",  false },  // Niger
  { "NU",  false },  // Niue
  { "NF",  false },  // Norfolk Island
  { "MP",  false },  // Northern Mariana Islands
  { "NO",  false },  // Norway
  { "OM",  true  },  // Oman
  { "PW",  false },  // Palau
  { "PS",  true  },  // Palestine
  { "PA",  true  },  // Panama
  { "PG",  false },  // Papua New Guinea
  { "PY",  true  },  // Paraguay
  { "PN",  false },  // Pitcairn
  { "PR",  true  },  // Puerto Rico
  { "QA",  true  },  // Qatar
  { "RW",  true  },  // Rwanda
  { "RE",  true  },  // Reunion
  { "BL",  false },  // Saint Barthelemy
  { "SH",  false },  // Saint Helena
  { "KN",  false },  // Saint Kitts and Nevis
  { "LC",  false },  // Saint Lucia
  { "MF",  false },  // Saint Martin
  { "PM",  false },  // Saint Pierre and Miquelon
  { "VC",  false },  // Saint Vincent and the Grenadines
  { "WS",  false },  // Samoa
  { "SM",  false },  // San Marino
  { "ST",  false },  // São Tomé and Príncipe
  { "SN",  true  },  // Senegal
  { "SC",  false },  // Seychelles
  { "SL",  false },  // Sierra Leone
  { "SX",  false },  // Sint Maarten
  { "SB",  false },  // Solomon Islands
  { "GS",  false },  // South Georgia and the South Sandwich Islands
  { "SS",  false },  // South Sudan
  { "LK",  true  },  // Sri Lanka
  { "SR",  false },  // Suriname
  { "SJ",  false },  // Svalbard and Jan Mayen
  { "SZ",  false },  // Swaziland
  { "TJ",  false },  // Tajikistan
  { "TZ",  true  },  // United Republic of Tanzania
  { "TL",  false },  // Timor-Leste
  { "TG",  false },  // Togo
  { "TK",  false },  // Tokelau
  { "TO",  false },  // Tonga
  { "TT",  true  },  // Trinidad and Tobago
  { "TN",  true  },  // Tunisia
  { "TM",  false },  // Turkmenistan
  { "TC",  false },  // Turks and Caicos Islands
  { "TV",  false },  // Tuvalu
  { "UG",  true  },  // Uganda
  { "UM",  false },  // United States Minor Outlying Islands
  { "UY",  true  },  // Uruguay
  { "UZ",  true  },  // Uzbekistan
  { "VU",  false },  // Vanuatu
  { "VG",  false },  // British Virgin Islands
  { "VI",  false },  // US Virgin Islands
  { "WF",  false },  // Wallis and Futuna
  { "EH",  false },  // Western Sahara
  { "YE",  false },  // Yemen
  { "ZM",  true  },  // Zambia
  { "ZW",  true  },  // Zimbabwe
  { "202", false },  // Sub-Saharan Africa
  { "014", false },  // Eastern Africa
  { "017", false },  // Middle Africa
  { "018", false },  // Southern Africa
  { "011", false },  // Western Africa
  { "013", false },  // Central America
  { "005", false },  // South America
  { "021", false },  // Northern America
  { "143", false },  // Central Asia
  { "035", false },  // South-eastern Asia
  { "150", false },  // Europe
  { "151", false },  // Eastern Europe
  { "154", false },  // Northern Europe
  { "039", false },  // Southern Europe
  { "155", false },  // Western Europe
  { "009", false },  // Oceania
  { "053", false },  // Australia and New Zealand
  { "054", false },  // Melanesia
  { "057", false },  // Micronesia
  { "061", false }   // Polynesia
};

const std::map<std::string, bool> kOtherCountryCodes = {
  { "US",  false },  // United States of America
  { "CA",  false },  // Canada
  { "GB",  false },  // United Kingdom (Great Britain and Northern Ireland)
  { "DE",  false },  // Germany
  { "FR",  false },  // France
  { "AU",  false },  // Australia
  { "NZ",  false },  // New Zealand
  { "IE",  false },  // Ireland
  { "AR",  false },  // Argentina
  { "AT",  false },  // Austria
  { "BR",  false },  // Brazil
  { "CH",  false },  // Switzerland
  { "CL",  false },  // Chile
  { "CO",  false },  // Colombia
  { "DK",  false },  // Denmark
  { "EC",  false },  // Ecuador
  { "IL",  false },  // Israel
  { "IN",  false },  // India
  { "IT",  false },  // Italy
  { "JP",  false },  // Japan
  { "KR",  false },  // Korea
  { "MX",  false },  // Mexico
  { "NL",  false },  // Netherlands
  { "PE",  false },  // Peru
  { "PH",  false },  // Philippines
  { "PL",  false },  // Poland
  { "SE",  false },  // Sweden
  { "SG",  false },  // Singapore
  { "VE",  false },  // Venezuela
  { "ZA",  false },  // South Africa
  { "KY",  false },  // Cayman Islands
  { "AE",  false },  // United Arab Emirates
  { "AL",  false },  // Albania
  { "AZ",  false },  // Azerbaijan
  { "BD",  false },  // Bangladesh
  { "BE",  false },  // Belgium
  { "BG",  false },  // Bulgaria
  { "CN",  false },  // China
  { "CZ",  false },  // Czechia
  { "DZ",  false },  // Algeria
  { "EG",  false },  // Egypt
  { "ES",  false },  // Spain
  { "FI",  false },  // Finland
  { "GR",  false },  // Greece
  { "HK",  false },  // Hong Kong
  { "HR",  false },  // Croatia
  { "HU",  false },  // Hungary
  { "ID",  false },  // Indonesia
  { "IQ",  false },  // Iraq
  { "KH",  false },  // Cambodia
  { "LT",  false },  // Lithuania
  { "MA",  false },  // Morocco
  { "MY",  false },  // Malaysia
  { "NG",  false },  // Nigeria
  { "NO",  false },  // Norway
  { "PK",  false },  // Pakistan
  { "PT",  false },  // Portugal
  { "RO",  false },  // Romania
  { "RS",  false },  // Serbia
  { "RU",  false },  // Russian Federation
  { "SA",  false },  // Saudi Arabia
  { "SI",  false },  // Slovenia
  { "SK",  false },  // Slovakia
  { "TH",  false },  // Thailand
  { "TR",  false },  // Turkey
  { "TW",  false },  // Taiwan
  { "UA",  false },  // Ukraine
  { "VN",  false },  // Vietnam
  { "AF",  false },  // Afghanistan
  { "AS",  true  },  // American Samoa
  { "AD",  false },  // Andorra
  { "AO",  false },  // Angola
  { "AI",  true  },  // Anguilla
  { "AQ",  true  },  // Antarctica
  { "AG",  true  },  // Antigua and Barbuda
  { "AM",  false },  // Armenia
  { "AW",  false },  // Aruba
  { "BS",  false },  // Bahamas
  { "BH",  false },  // Bahrain
  { "BB",  false },  // Barbados
  { "BY",  false },  // Belarus
  { "BZ",  false },  // Belize
  { "BJ",  false },  // Benin
  { "BM",  false },  // Bermuda
  { "BT",  false },  // Bhutan
  { "BO",  false },  // Bolivia
  { "BQ",  true  },  // Bonaire
  { "BA",  false },  // Bosnia and Herzegovina
  { "BW",  false },  // Botswana
  { "BV",  true  },  // Bouvet Island
  { "IO",  true  },  // British Indian Ocean Territory
  { "BN",  false },  // Brunei Darussalam
  { "BF",  false },  // Burkina Faso
  { "BI",  false },  // Burundi
  { "CM",  false },  // Cameroon
  { "CV",  false },  // Cape Verde
  { "TD",  true  },  // Chad
  { "CX",  true  },  // Christmas Island
  { "CC",  true  },  // Cocos (Keeling) Islands
  { "KM",  true  },  // Comoros
  { "CG",  false },  // Republic of the Congo
  { "CD",  false },  // Democratic Republic of the Congo
  { "CK",  true  },  // Cook Islands
  { "CR",  false },  // Costa Rica
  { "CW",  false },  // Curacao
  { "CY",  false },  // Cyprus
  { "CI",  false },  // Cote d'Ivoire
  { "DJ",  false },  // Djibouti
  { "DM",  false },  // Dominica
  { "DO",  false },  // Dominican Republic
  { "SV",  false },  // El Salvador
  { "GQ",  true  },  // Equatorial Guinea
  { "ER",  true  },  // Eritrea
  { "EE",  false },  // Estonia
  { "ET",  false },  // Ethiopia
  { "FK",  true  },  // Falkland Islands
  { "FO",  false },  // Faroe Islands
  { "FJ",  false },  // Fiji
  { "GF",  false },  // French Guiana
  { "PF",  false },  // French Polynesia
  { "TF",  true  },  // French and Antarctic Lands
  { "GA",  false },  // Gabon
  { "GM",  false },  // Gambia
  { "GE",  false },  // Georgia
  { "GH",  false },  // Ghana
  { "GI",  false },  // Gibraltar
  { "GL",  true  },  // Greenland
  { "GD",  false },  // Grenada
  { "GP",  false },  // Guadeloupe
  { "GU",  false },  // Guam
  { "GT",  false },  // Guatemala
  { "GG",  false },  // Guernsey
  { "GN",  false },  // Guinea
  { "GW",  true  },  // Guinea-Bissau
  { "GY",  false },  // Guyana
  { "HT",  false },  // Haiti
  { "HM",  true  },  // Heard Island and McDonald Islands
  { "VA",  true  },  // Vatican City
  { "HN",  false },  // Honduras
  { "IS",  false },  // Iceland
  { "IM",  false },  // Isle of Man
  { "JM",  false },  // Jamaica
  { "JE",  false },  // Jersey
  { "JO",  false },  // Jordan
  { "KZ",  false },  // Kazakhstan
  { "KE",  false },  // Kenya
  { "KI",  true  },  // Kiribati
  { "KW",  false },  // Kuwait
  { "KG",  false },  // Kyrgyzstan
  { "LA",  false },  // Lao People's Democratic Republic
  { "LV",  false },  // Latvia
  { "LB",  false },  // Lebanon
  { "LS",  false },  // Lesotho
  { "LR",  true  },  // Liberia
  { "LI",  false },  // Liechtenstein
  { "LU",  false },  // Luxembourg
  { "MO",  false },  // Macao
  { "MK",  false },  // Macedonia
  { "MG",  false },  // Madagascar
  { "MW",  false },  // Malawi
  { "MV",  false },  // Maldives
  { "ML",  false },  // Mali
  { "MT",  false },  // Malta
  { "MH",  true  },  // Marshall Islands
  { "MQ",  false },  // Martinique
  { "MR",  false },  // Mauritania
  { "MU",  false },  // Mauritius
  { "YT",  true  },  // Mayotte
  { "FM",  true  },  // Micronesia, Federated States of
  { "MD",  false },  // Moldova
  { "MC",  false },  // Monaco
  { "MN",  false },  // Mongolia
  { "ME",  false },  // Montenegro
  { "MS",  true  },  // Montserrat
  { "MZ",  false },  // Mozambique
  { "MM",  false },  // Myanmar
  { "NA",  false },  // Namibia
  { "NR",  true  },  // Nauru
  { "NP",  false },  // Nepal
  { "NC",  false },  // New Caledonia
  { "NI",  false },  // Nicaragua
  { "NE",  false },  // Niger
  { "NU",  true  },  // Niue
  { "NF",  true  },  // Norfolk Island
  { "MP",  true  },  // Northern Mariana Islands
  { "NO",  false },  // Norway
  { "OM",  false },  // Oman
  { "PW",  true  },  // Palau
  { "PS",  false },  // Palestine
  { "PA",  false },  // Panama
  { "PG",  false },  // Papua New Guinea
  { "PY",  false },  // Paraguay
  { "PN",  true  },  // Pitcairn
  { "PR",  false },  // Puerto Rico
  { "QA",  false },  // Qatar
  { "RW",  false },  // Rwanda
  { "RE",  false },  // Reunion
  { "BL",  true  },  // Saint Barthelemy
  { "SH",  true  },  // Saint Helena
  { "KN",  true  },  // Saint Kitts and Nevis
  { "LC",  false },  // Saint Lucia
  { "MF",  true  },  // Saint Martin
  { "PM",  true  },  // Saint Pierre and Miquelon
  { "VC",  false },  // Saint Vincent and the Grenadines
  { "WS",  true  },  // Samoa
  { "SM",  true  },  // San Marino
  { "ST",  true  },  // São Tomé and Príncipe
  { "SN",  false },  // Senegal
  { "SC",  false },  // Seychelles
  { "SL",  false },  // Sierra Leone
  { "SX",  true  },  // Sint Maarten
  { "SB",  true  },  // Solomon Islands
  { "GS",  true  },  // South Georgia and the South Sandwich Islands
  { "SS",  true  },  // South Sudan
  { "LK",  false },  // Sri Lanka
  { "SR",  false },  // Suriname
  { "SJ",  true  },  // Svalbard and Jan Mayen
  { "SZ",  false },  // Swaziland
  { "TJ",  true  },  // Tajikistan
  { "TZ",  false },  // United Republic of Tanzania
  { "TL",  true  },  // Timor-Leste
  { "TG",  false },  // Togo
  { "TK",  true  },  // Tokelau
  { "TO",  false },  // Tonga
  { "TT",  false },  // Trinidad and Tobago
  { "TN",  false },  // Tunisia
  { "TM",  true  },  // Turkmenistan
  { "TC",  true  },  // Turks and Caicos Islands
  { "TV",  true  },  // Tuvalu
  { "UG",  false },  // Uganda
  { "UM",  true  },  // United States Minor Outlying Islands
  { "UY",  false },  // Uruguay
  { "UZ",  false },  // Uzbekistan
  { "VU",  true  },  // Vanuatu
  { "VG",  true  },  // British Virgin Islands
  { "VI",  false },  // US Virgin Islands
  { "WF",  true  },  // Wallis and Futuna
  { "EH",  true  },  // Western Sahara
  { "YE",  false },  // Yemen
  { "ZM",  false },  // Zambia
  { "ZW",  false },  // Zimbabwe
  { "202", false },  // Sub-Saharan Africa
  { "014", false },  // Eastern Africa
  { "017", false },  // Middle Africa
  { "018", false },  // Southern Africa
  { "011", false },  // Western Africa
  { "013", false },  // Central America
  { "005", false },  // South America
  { "021", false },  // Northern America
  { "143", false },  // Central Asia
  { "035", false },  // South-eastern Asia
  { "150", false },  // Europe
  { "151", false },  // Eastern Europe
  { "154", false },  // Northern Europe
  { "039", false },  // Southern Europe
  { "155", false },  // Western Europe
  { "009", false },  // Oceania
  { "053", false },  // Australia and New Zealand
  { "054", false },  // Melanesia
  { "057", false },  // Micronesia
  { "061", false }   // Polynesia
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_STATIC_VALUES_H_
