/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_L10N_COMMON_OFAC_SANCTIONED_ISO_3166_1_COUNTRY_CODE_CONSTANTS_H_
#define BRAVE_COMPONENTS_L10N_COMMON_OFAC_SANCTIONED_ISO_3166_1_COUNTRY_CODE_CONSTANTS_H_

#include "base/containers/fixed_flat_set.h"
#include "base/strings/string_piece.h"

namespace brave_l10n {

// See https://orpa.princeton.edu/export-controls/sanctioned-countries.

constexpr auto kOFACSactionedISO31661CountryCodes =
    base::MakeFixedFlatSet<base::StringPiece>({
        // List of Comprehensively Sanctioned Countries. Most transactions,
        // including those involving persons or entities "ordinarily resident"
        // in the following countries, require an Office of Foreign Assets
        // Control (OFAC) License.
        //
        // NOTE: This list excludes Crimea, Donetsk, and Luhansk regions of
        // Ukraine because they are all listed under UA ISO 3166-1 alpha-2 and
        // UKR ISO 3166-1 alpha-3 country codes.

        // ISO 3166-1 alpha-2 country codes. See
        // https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2.
        "CU",  // Cuba
        "IR",  // Iran
        "KP",  // North Korea
        "RU",  // Russia
        "SY",  // Syria

        // ISO 3166-1 alpha-3 country codes. See
        // https://en.wikipedia.org/wiki/ISO_3166-1_alpha-3.
        "CUB",  // Cuba
        "IRN",  // Iran
        "PRK",  // North Korea
        "RUS",  // Russia
        "SYR",  // Syria

        // ISO 3166-1 numeric-3 country codes. See
        // https://en.wikipedia.org/wiki/ISO_3166-1_numeric.
        "192",  // Cuba
        "364",  // Iran
        "408",  // North Korea
        "643",  // Russia
        "760"   // Syria
    });

}  // namespace brave_l10n

#endif  // BRAVE_COMPONENTS_L10N_COMMON_OFAC_SANCTIONED_ISO_3166_1_COUNTRY_CODE_CONSTANTS_H_
