/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/locale/country_code_util.h"

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"

namespace {

constexpr auto kCountryCodeAnonymitySet =
    base::MakeFixedFlatSet<base::StringPiece>({
        "US",  // United States of America
        "CA",  // Canada
        "GB",  // United Kingdom (Great Britain and Northern Ireland)
        "DE",  // Germany
        "FR",  // France
        "AU",  // Australia
        "NZ",  // New Zealand
        "IE",  // Ireland
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
        "ZA",  // South Africa
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
        "RU",  // Russian Federation
        "SA",  // Saudi Arabia
        "SI",  // Slovenia
        "SK",  // Slovakia
        "TH",  // Thailand
        "TR",  // Turkey
        "TW",  // Taiwan
        "UA",  // Ukraine
        "VN",  // Vietnam
        "AF",  // Afghanistan
        "AM",  // Armenia
        "BS",  // Bahamas
        "BH",  // Bahrain
        "BB",  // Barbados
        "BY",  // Belarus
        "BJ",  // Benin
        "BO",  // Bolivia
        "BA",  // Bosnia and Herzegovina
        "BW",  // Botswana
        "BN",  // Brunei Darussalam
        "CM",  // Cameroon
        "CD",  // Democratic Republic of the Congo
        "CR",  // Costa Rica
        "CY",  // Cyprus
        "CI",  // Cote d'Ivoire
        "DO",  // Dominican Republic
        "SV",  // El Salvador
        "EE",  // Estonia
        "ET",  // Ethiopia
        "GE",  // Georgia
        "GH",  // Ghana
        "GT",  // Guatemala
        "HN",  // Honduras
        "IS",  // Iceland
        "JM",  // Jamaica
        "JO",  // Jordan
        "KZ",  // Kazakhstan
        "KE",  // Kenya
        "KW",  // Kuwait
        "KG",  // Kyrgyzstan
        "LV",  // Latvia
        "LB",  // Lebanon
        "LU",  // Luxembourg
        "MK",  // Macedonia
        "MG",  // Madagascar
        "MT",  // Malta
        "MU",  // Mauritius
        "MD",  // Moldova
        "MN",  // Mongolia
        "ME",  // Montenegro
        "MM",  // Myanmar
        "NA",  // Namibia
        "NP",  // Nepal
        "NI",  // Nicaragua
        "OM",  // Oman
        "PS",  // Palestine
        "PA",  // Panama
        "PY",  // Paraguay
        "PR",  // Puerto Rico
        "QA",  // Qatar
        "RW",  // Rwanda
        "RE",  // Reunion
        "SN",  // Senegal
        "LK",  // Sri Lanka
        "TZ",  // United Republic of Tanzania
        "TT",  // Trinidad and Tobago
        "TN",  // Tunisia
        "UG",  // Uganda
        "UY",  // Uruguay
        "UZ",  // Uzbekistan
        "ZM",  // Zambia
        "ZW"   // Zimbabwe
    });

constexpr auto kOtherCountryCodes = base::MakeFixedFlatSet<base::StringPiece>({
    "AS",  // American Samoa
    "AI",  // Anguilla
    "AQ",  // Antarctica
    "AG",  // Antigua and Barbuda
    "BQ",  // Bonaire
    "BV",  // Bouvet Island
    "IO",  // British Indian Ocean Territory
    "TD",  // Chad
    "CX",  // Christmas Island
    "CC",  // Cocos (Keeling) Islands
    "KM",  // Comoros
    "CK",  // Cook Islands
    "GQ",  // Equatorial Guinea
    "ER",  // Eritrea
    "FK",  // Falkland Islands
    "TF",  // French and Antarctic Lands
    "GL",  // Greenland
    "GW",  // Guinea-Bissau
    "HM",  // Heard Island and McDonald Islands
    "VA",  // Vatican City
    "KI",  // Kiribati
    "LR",  // Liberia
    "MH",  // Marshall Islands
    "YT",  // Mayotte
    "FM",  // Micronesia, Federated States of
    "MS",  // Montserrat
    "NR",  // Nauru
    "NU",  // Niue
    "NF",  // Norfolk Island
    "MP",  // Northern Mariana Islands
    "PW",  // Palau
    "PN",  // Pitcairn
    "BL",  // Saint Barthelemy
    "SH",  // Saint Helena
    "KN",  // Saint Kitts and Nevis
    "MF",  // Saint Martin
    "PM",  // Saint Pierre and Miquelon
    "WS",  // Samoa
    "SM",  // San Marino
    "ST",  // São Tomé and Príncipe
    "SX",  // Saint Maarten
    "SB",  // Solomon Islands
    "GS",  // South Georgia and the South Sandwich Islands
    "SS",  // South Sudan
    "SJ",  // Svalbard and Jan Mayen
    "TJ",  // Tajikistan
    "TL",  // Timor-Leste
    "TK",  // Tokelau
    "TM",  // Turkmenistan
    "TC",  // Turks and Caicos Islands
    "TV",  // Tuvalu
    "UM",  // United States Minor Outlying Islands
    "VU",  // Vanuatu
    "VG",  // British Virgin Islands
    "WF",  // Wallis and Futuna
    "EH"   // Western Sahara
});

}  // namespace

namespace ads::privacy::locale {

bool IsCountryCodeMemberOfAnonymitySet(const std::string& country_code) {
  return base::Contains(kCountryCodeAnonymitySet,
                        base::ToUpperASCII(country_code));
}

bool ShouldClassifyCountryCodeAsOther(const std::string& country_code) {
  return base::Contains(kOtherCountryCodes, base::ToUpperASCII(country_code));
}

}  // namespace ads::privacy::locale
