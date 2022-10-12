/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_L10N_COMMON_UN_M49_CODE_CONSTANTS_H_
#define BRAVE_COMPONENTS_L10N_COMMON_UN_M49_CODE_CONSTANTS_H_

#include "base/containers/fixed_flat_set.h"
#include "base/strings/string_piece.h"

namespace brave_l10n {

// See https://en.wikipedia.org/wiki/UN_M49.

constexpr auto kUnM49Codes = base::MakeFixedFlatSet<base::StringPiece>({
    "001",  // World
    "002",  // Africa
    "003",  // North America
    "005",  // South America
    "009",  // Oceania
    "010",  // Antarctica
    "011",  // Western Africa
    "013",  // Central America
    "014",  // Eastern Africa
    "015",  // Northern Africa
    "017",  // Middle Africa
    "018",  // Southern Africa
    "019",  // Americas
    "021",  // Northern America
    "029",  // Caribbean
    "030",  // Eastern Asia
    "034",  // Southern Asia
    "035",  // South-eastern Asia
    "039",  // Southern Europe
    "053",  // Australia and New Zealand
    "054",  // Melanesia
    "057",  // Micronesia
    "061",  // Polynesia
    "142",  // Asia
    "143",  // Central Asia
    "145",  // Western Asia
    "150",  // Europe
    "151",  // Eastern Europe (including Northern Asia)
    "154",  // Northern Europe
    "155",  // Western Europe
    "202",  // Sub-Saharan Africa
    "419",  // Latin America and the Caribbean
    "830"   // Channel Islands
});

}  // namespace brave_l10n

#endif  // BRAVE_COMPONENTS_L10N_COMMON_UN_M49_CODE_CONSTANTS_H_
