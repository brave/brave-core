/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_L10N_COMMON_OFAC_SANCTIONED_ISO_3166_1_COUNTRY_CODE_CONSTANTS_H_
#define BRAVE_COMPONENTS_L10N_COMMON_OFAC_SANCTIONED_ISO_3166_1_COUNTRY_CODE_CONSTANTS_H_

#include <string_view>

#include "base/containers/fixed_flat_set.h"

namespace brave_l10n {

// See https://orpa.princeton.edu/export-controls/sanctioned-countries.

inline constexpr auto kOFACSactionedISO31661CountryCodes =
    base::MakeFixedFlatSet<std::string_view>({
        // List of Comprehensively Sanctioned Countries. Most transactions,
        // including those involving persons or entities "ordinarily resident"
        // in the following countries, require an Office of Foreign Assets
        // Control (OFAC) License.

        // ISO 3166-1 alpha-2 country codes. See
        // https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2.
        "BY",  // Belarus
        "CU",  // Cuba
        "IR",  // Iran
        "KP",  // North Korea
        "MD",  // Moldova
        "RU",  // Russia
        "SY",  // Syria
        "UA",  // Ukraine (includes Crimea, Donetsk, and Luhansk regions)

        // ISO 3166-1 alpha-3 country codes. See
        // https://en.wikipedia.org/wiki/ISO_3166-1_alpha-3.
        "BLR",  // Belarus
        "CUB",  // Cuba
        "IRN",  // Iran
        "MDA",  // Moldova
        "PRK",  // North Korea
        "RUS",  // Russia
        "SYR",  // Syria
        "UKR",  // Ukraine (includes Crimea, Donetsk, and Luhansk regions)

        // ISO 3166-1 numeric-3 country codes. See
        // https://en.wikipedia.org/wiki/ISO_3166-1_numeric.
        "112",  // Belarus
        "192",  // Cuba
        "364",  // Iran
        "408",  // North Korea
        "498",  // Moldova
        "643",  // Russia
        "760",  // Syria
        "804"   // Ukraine (includes Crimea, Donetsk, and Luhansk regions)
    });

}  // namespace brave_l10n

#endif  // BRAVE_COMPONENTS_L10N_COMMON_OFAC_SANCTIONED_ISO_3166_1_COUNTRY_CODE_CONSTANTS_H_
