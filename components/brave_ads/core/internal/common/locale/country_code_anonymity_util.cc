/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/locale/country_code_anonymity_util.h"

#include <string_view>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/strings/string_util.h"

namespace {

constexpr auto kCountryCodeAnonymitySet =
    base::MakeFixedFlatSet<std::string_view>(
        base::sorted_unique,
        {
            "AE",  // United Arab Emirates
            "AF",  // Afghanistan
            "AL",  // Albania
            "AM",  // Armenia
            "AR",  // Argentina
            "AT",  // Austria
            "AU",  // Australia
            "AZ",  // Azerbaijan
            "BA",  // Bosnia and Herzegovina
            "BB",  // Barbados
            "BD",  // Bangladesh
            "BE",  // Belgium
            "BG",  // Bulgaria
            "BH",  // Bahrain
            "BJ",  // Benin
            "BN",  // Brunei Darussalam
            "BO",  // Bolivia
            "BR",  // Brazil
            "BS",  // Bahamas
            "BW",  // Botswana
            "BY",  // Belarus
            "CA",  // Canada
            "CD",  // Democratic Republic of the Congo
            "CH",  // Switzerland
            "CI",  // Cote d'Ivoire
            "CL",  // Chile
            "CM",  // Cameroon
            "CN",  // China
            "CO",  // Colombia
            "CR",  // Costa Rica
            "CY",  // Cyprus
            "CZ",  // Czechia
            "DE",  // Germany
            "DK",  // Denmark
            "DO",  // Dominican Republic
            "DZ",  // Algeria
            "EC",  // Ecuador
            "EE",  // Estonia
            "EG",  // Egypt
            "ES",  // Spain
            "ET",  // Ethiopia
            "FI",  // Finland
            "FR",  // France
            "GB",  // United Kingdom (Great Britain and Northern Ireland)
            "GE",  // Georgia
            "GH",  // Ghana
            "GR",  // Greece
            "GT",  // Guatemala
            "HK",  // Hong Kong
            "HN",  // Honduras
            "HR",  // Croatia
            "HU",  // Hungary
            "ID",  // Indonesia
            "IE",  // Ireland
            "IL",  // Israel
            "IN",  // India
            "IQ",  // Iraq
            "IS",  // Iceland
            "IT",  // Italy
            "JM",  // Jamaica
            "JO",  // Jordan
            "JP",  // Japan
            "KE",  // Kenya
            "KG",  // Kyrgyzstan
            "KH",  // Cambodia
            "KR",  // Korea
            "KW",  // Kuwait
            "KZ",  // Kazakhstan
            "LB",  // Lebanon
            "LK",  // Sri Lanka
            "LT",  // Lithuania
            "LU",  // Luxembourg
            "LV",  // Latvia
            "MA",  // Morocco
            "MD",  // Moldova
            "ME",  // Montenegro
            "MG",  // Madagascar
            "MK",  // Macedonia
            "MM",  // Myanmar
            "MN",  // Mongolia
            "MT",  // Malta
            "MU",  // Mauritius
            "MX",  // Mexico
            "MY",  // Malaysia
            "NA",  // Namibia
            "NG",  // Nigeria
            "NI",  // Nicaragua
            "NL",  // Netherlands
            "NO",  // Norway
            "NP",  // Nepal
            "NZ",  // New Zealand
            "OM",  // Oman
            "PA",  // Panama
            "PE",  // Peru
            "PH",  // Philippines
            "PK",  // Pakistan
            "PL",  // Poland
            "PR",  // Puerto Rico
            "PS",  // Palestine
            "PT",  // Portugal
            "PY",  // Paraguay
            "QA",  // Qatar
            "RE",  // Reunion
            "RO",  // Romania
            "RS",  // Serbia
            "RU",  // Russia
            "RW",  // Rwanda
            "SA",  // Saudi Arabia
            "SE",  // Sweden
            "SG",  // Singapore
            "SI",  // Slovenia
            "SK",  // Slovakia
            "SN",  // Senegal
            "SV",  // El Salvador
            "TH",  // Thailand
            "TN",  // Tunisia
            "TR",  // Turkey
            "TT",  // Trinidad and Tobago
            "TW",  // Taiwan
            "TZ",  // United Republic of Tanzania
            "UA",  // Ukraine
            "UG",  // Uganda
            "US",  // United States of America
            "UY",  // Uruguay
            "UZ",  // Uzbekistan
            "VE",  // Venezuela
            "VN",  // Vietnam
            "ZA",  // South Africa
            "ZM",  // Zambia
            "ZW",  // Zimbabw
        });

constexpr auto kOtherCountryCodes = base::MakeFixedFlatSet<std::string_view>(
    base::sorted_unique,
    {
        "AG",  // "Antigua and Barbuda
        "AI",  // "Anguilla
        "AQ",  // "Antarctica
        "AS",  // "American Samoa
        "BL",  // "Saint Barthelemy
        "BQ",  // "Bonaire
        "BV",  // "Bouvet Island
        "CC",  // "Cocos (Keeling) Islands
        "CK",  // "Cook Islands
        "CX",  // "Christmas Island
        "EH",  // "Western Sahara
        "ER",  // "Eritrea
        "FK",  // "Falkland Islands
        "FM",  // "Micronesia, Federated States of
        "GL",  // "Greenland
        "GQ",  // "Equatorial Guinea
        "GS",  // "South Georgia and the South Sandwich Islands
        "GW",  // "Guinea-Bissau
        "HM",  // "Heard Island and McDonald Islands
        "IO",  // "British Indian Ocean Territory
        "KI",  // "Kiribati
        "KM",  // "Comoros
        "KN",  // "Saint Kitts and Nevis
        "LR",  // "Liberia
        "MF",  // "Saint Martin
        "MH",  // "Marshall Islands
        "MP",  // "Northern Mariana Islands
        "MS",  // "Montserrat
        "NF",  // "Norfolk Island
        "NR",  // "Nauru
        "NU",  // "Niue
        "PM",  // "Saint Pierre and Miquelon
        "PN",  // "Pitcairn
        "PW",  // "Palau
        "SB",  // "Solomon Islands
        "SH",  // "Saint Helena
        "SJ",  // "Svalbard and Jan Mayen
        "SM",  // "San Marino
        "SS",  // "South Sudan
        "ST",  // "São Tomé and Príncipe
        "SX",  // "Saint Maarten
        "TC",  // "Turks and Caicos Islands
        "TD",  // "Chad
        "TF",  // "French and Antarctic Lands
        "TJ",  // "Tajikistan
        "TK",  // "Tokelau
        "TL",  // "Timor-Leste
        "TM",  // "Turkmenistan
        "TV",  // "Tuvalu
        "UM",  // "United States Minor Outlying Islands
        "VA",  // "Vatican City
        "VG",  // "British Virgin Islands
        "VU",  // "Vanuatu
        "WF",  // "Wallis and Futuna
        "WS",  // "Samoa
        "YT",  // "Mayotte
    });

}  // namespace

namespace brave_ads {

bool IsCountryCodeMemberOfAnonymitySet(const std::string& country_code) {
  return base::Contains(kCountryCodeAnonymitySet,
                        base::ToUpperASCII(country_code));
}

bool ShouldClassifyCountryCodeAsOther(const std::string& country_code) {
  return base::Contains(kOtherCountryCodes, base::ToUpperASCII(country_code));
}

}  // namespace brave_ads
