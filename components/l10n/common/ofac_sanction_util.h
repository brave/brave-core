/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_L10N_COMMON_OFAC_SANCTION_UTIL_H_
#define BRAVE_COMPONENTS_L10N_COMMON_OFAC_SANCTION_UTIL_H_

#include <string_view>

namespace brave_l10n {

// Returns |true| if the given |country_code| is on the OFAC sanctioned list,
// otherwise |false|. |country_code| supports ISO 3166-1 alpha-2, ISO 3166-1
// alpha-3, and ISO 3166-1 number-3 country codes. See
// https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2,
// https://en.wikipedia.org/wiki/ISO_3166-1_alpha-3, and
// https://en.wikipedia.org/wiki/ISO_3166-1_numeric.
bool IsISOCountryCodeOFACSanctioned(std::string_view country_code);

// Returns |true| if the given |code| is on the OFAC sanctioned list, otherwise
// |false|. |code| supports UN M.49 codes. See
// https://en.wikipedia.org/wiki/UN_M49.
//
// NOTE: Sanctioning UN M.49 codes will also block all other countries that are
// part of the same code.
bool IsUNM49CodeOFACSanctioned(std::string_view code);

}  // namespace brave_l10n

#endif  // BRAVE_COMPONENTS_L10N_COMMON_OFAC_SANCTION_UTIL_H_
