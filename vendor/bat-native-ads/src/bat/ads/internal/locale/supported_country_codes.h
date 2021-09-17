/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LOCALE_SUPPORTED_COUNTRY_CODES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LOCALE_SUPPORTED_COUNTRY_CODES_H_

#include "bat/ads/internal/locale/supported_country_codes_aliases.h"

namespace ads {

const SupportedCountryCodesMap kSupportedCountryCodes = {
    // Append newly supported country codes with a new schema version and update
    // |kSupportedCountryCodesSchemaVersionNumber| to match the new version

    // |kCountryCodeAnonymitySet| and |kOtherCountryCodes| in
    // |bat-native-ads/src/bat/ads/internal/locale/country_codes.h| must be
    // updated to reflect newly supported regions

    //   Format: { schema_version : { country code... } }
    {1,
     {
         "US",  // United States of America
         "CA",  // Canada
         "GB",  // United Kingdom (Great Britain and Northern Ireland)
         "DE",  // Germany
         "FR"   // France
     }},
    {2,
     {
         "AU",  // Australia
         "NZ",  // New Zealand
         "IE"   // Ireland
     }},
    {3,
     {
         "AR",  // Argentina
         "AT",  // Austria
         "BR",  // Brazil
         "CH",  // Switzerland
         "CL",  // Chile
         "CO",  // Colombia
         "DK",  // Denmark
         "EC",  // Ecuador
         "IL",  // Israel
         "IN",  // India
         "IT",  // Italy
         "JP",  // Japan
         "KR",  // Korea
         "MX",  // Mexico
         "NL",  // Netherlands
         "PE",  // Peru
         "PH",  // Philippines
         "PL",  // Poland
         "SE",  // Sweden
         "SG",  // Singapore
         "VE",  // Venezuela
         "ZA"   // South Africa
     }},
    {4,
     {
         "KY"  // Cayman Islands
     }},
    {5,
     {
         "AE",  // United Arab Emirates
         "AL",  // Albania
         "AZ",  // Azerbaijan
         "BD",  // Bangladesh
         "BE",  // Belgium
         "BG",  // Bulgaria
         "CN",  // China
         "CZ",  // Czechia
         "DZ",  // Algeria
         "EG",  // Egypt
         "ES",  // Spain
         "FI",  // Finland
         "GR",  // Greece
         "HK",  // Hong Kong
         "HR",  // Croatia
         "HU",  // Hungary
         "ID",  // Indonesia
         "IQ",  // Iraq
         "KH",  // Cambodia
         "LT",  // Lithuania
         "MA",  // Morocco
         "MY",  // Malaysia
         "NG",  // Nigeria
         "NO",  // Norway
         "PK",  // Pakistan
         "PT",  // Portugal
         "RO",  // Romania
         "RS",  // Serbia
         "RU",  // Russia
         "SA",  // Saudi Arabia
         "SI",  // Slovenia
         "SK",  // Slovakia
         "TH",  // Thailand
         "TR",  // Turkey
         "TW",  // Taiwan
         "UA",  // Ukraine
         "VN"   // Vietnam
     }},
    {6,
     {
         "AF",  // Afghanistan
         "AS",  // American Samoa
         "AD",  // Andorra
         "AO",  // Angola
         "AI",  // Anguilla
         "AQ",  // Antarctica
         "AG",  // Antigua and Barbuda
         "AM",  // Armenia
         "AW",  // Aruba
         "BS",  // Bahamas
         "BH",  // Bahrain
         "BB",  // Barbados
         "BY",  // Belarus
         "BZ",  // Belize
         "BJ",  // Benin
         "BM",  // Bermuda
         "BT",  // Bhutan
         "BO",  // Bolivia
         "BQ",  // Bonaire
         "BA",  // Bosnia and Herzegovina
         "BW",  // Botswana
         "BV",  // Bouvet Island
         "IO",  // British Indian Ocean Territory
         "BN",  // Brunei Darussalam
         "BF",  // Burkina Faso
         "BI",  // Burundi
         "CM",  // Cameroon
         "CV",  // Cape Verde
         "TD",  // Chad
         "CX",  // Christmas Island
         "CC",  // Cocos (Keeling) Islands
         "KM",  // Comoros
         "CG",  // Republic of the Congo
         "CD",  // Democratic Republic of the Congo
         "CK",  // Cook Islands
         "CR",  // Costa Rica
         "CW",  // Curacao
         "CY",  // Cyprus
         "CI",  // Cote d'Ivoire
         "DJ",  // Djibouti
         "DM",  // Dominica
         "DO",  // Dominican Republic
         "SV",  // El Salvador
         "GQ",  // Equatorial Guinea
         "ER",  // Eritrea
         "EE",  // Estonia
         "ET",  // Ethiopia
         "FK",  // Falkland Islands
         "FO",  // Faroe Islands
         "FJ",  // Fiji
         "GF",  // French Guiana
         "PF",  // French Polynesia
         "TF",  // French and Antarctic Lands
         "GA",  // Gabon
         "GM",  // Gambia
         "GE",  // Georgia
         "GH",  // Ghana
         "GI",  // Gibraltar
         "GL",  // Greenland
         "GD",  // Grenada
         "GP",  // Guadeloupe
         "GU",  // Guam
         "GT",  // Guatemala
         "GG",  // Guernsey
         "GN",  // Guinea
         "GW",  // Guinea-Bissau
         "GY",  // Guyana
         "HT",  // Haiti
         "HM",  // Heard Island and McDonald Islands
         "VA",  // Vatican City
         "HN",  // Honduras
         "IS",  // Iceland
         "IM",  // Isle of Man
         "JM",  // Jamaica
         "JE",  // Jersey
         "JO",  // Jordan
         "KZ",  // Kazakhstan
         "KE",  // Kenya
         "KI",  // Kiribati
         "KW",  // Kuwait
         "KG",  // Kyrgyzstan
         "LA",  // Lao People's Democratic Republic
         "LV",  // Latvia
         "LB",  // Lebanon
         "LS",  // Lesotho
         "LR",  // Liberia
         "LI",  // Liechtenstein
         "LU",  // Luxembourg
         "MO",  // Macao
         "MK",  // Macedonia
         "MG",  // Madagascar
         "MW",  // Malawi
         "MV",  // Maldives
         "ML",  // Mali
         "MT",  // Malta
         "MH",  // Marshall Islands
         "MQ",  // Martinique
         "MR",  // Mauritania
         "MU",  // Mauritius
         "YT",  // Mayotte
         "FM",  // Micronesia, Federated States of
         "MD",  // Moldova
         "MC",  // Monaco
         "MN",  // Mongolia
         "ME",  // Montenegro
         "MS",  // Montserrat
         "MZ",  // Mozambique
         "MM",  // Myanmar
         "NA",  // Namibia
         "NR",  // Nauru
         "NP",  // Nepal
         "NC",  // New Caledonia
         "NI",  // Nicaragua
         "NE",  // Niger
         "NU",  // Niue
         "NF",  // Norfolk Island
         "MP",  // Northern Mariana Islands
         "NO",  // Norway
         "OM",  // Oman
         "PW",  // Palau
         "PS",  // Palestine
         "PA",  // Panama
         "PG",  // Papua New Guinea
         "PY",  // Paraguay
         "PN",  // Pitcairn
         "PR",  // Puerto Rico
         "QA",  // Qatar
         "RW",  // Rwanda
         "RE",  // Reunion
         "BL",  // Saint Barthelemy
         "SH",  // Saint Helena
         "KN",  // Saint Kitts and Nevis
         "LC",  // Saint Lucia
         "MF",  // Saint Martin
         "PM",  // Saint Pierre and Miquelon
         "VC",  // Saint Vincent and the Grenadines
         "WS",  // Samoa
         "SM",  // San Marino
         "ST",  // São Tomé and Príncipe
         "SN",  // Senegal
         "SC",  // Seychelles
         "SL",  // Sierra Leone
         "SX",  // Sint Maarten
         "SB",  // Solomon Islands
         "GS",  // South Georgia and the South Sandwich Islands
         "SS",  // South Sudan
         "LK",  // Sri Lanka
         "SR",  // Suriname
         "SJ",  // Svalbard and Jan Mayen
         "SZ",  // Swaziland
         "TJ",  // Tajikistan
         "TZ",  // United Republic of Tanzania
         "TL",  // Timor-Leste
         "TG",  // Togo
         "TK",  // Tokelau
         "TO",  // Tonga
         "TT",  // Trinidad and Tobago
         "TN",  // Tunisia
         "TM",  // Turkmenistan
         "TC",  // Turks and Caicos Islands
         "TV",  // Tuvalu
         "UG",  // Uganda
         "UM",  // United States Minor Outlying Islands
         "UY",  // Uruguay
         "UZ",  // Uzbekistan
         "VU",  // Vanuatu
         "VG",  // British Virgin Islands
         "VI",  // US Virgin Islands
         "WF",  // Wallis and Futuna
         "EH",  // Western Sahara
         "YE",  // Yemen
         "ZM",  // Zambia
         "ZW"   // Zimbabwe
     }},
    {7,
     {
         "150",  // Europe
     }},
    {8,
     {
         "202",  // Sub-Saharan Africa
         "014",  // Eastern Africa
         "017",  // Middle Africa
         "018",  // Southern Africa
         "011",  // Western Africa
         "013",  // Central America
         "005",  // South America
         "021",  // Northern America
         "143",  // Central Asia
         "035",  // South-eastern Asia
         "151",  // Eastern Europe
         "154",  // Northern Europe
         "039",  // Southern Europe
         "155",  // Western Europe
         "009",  // Oceania
         "053",  // Australia and New Zealand
         "054",  // Melanesia
         "057",  // Micronesia
         "061"   // Polynesia
     }},
    {9,
     {
         "830"  // Channel Islands
     }}

    // IMPORTANT: When adding new schema versions |newly_supported_locale_| must
    // be updated in |BraveAdsBrowserTest| to reflect a locale from the latest
    // schema version
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LOCALE_SUPPORTED_COUNTRY_CODES_H_
