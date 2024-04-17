/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_L10N_COMMON_OFAC_SANCTIONED_UN_M49_CODE_CONSTANTS_H_
#define BRAVE_COMPONENTS_L10N_COMMON_OFAC_SANCTIONED_UN_M49_CODE_CONSTANTS_H_

#include <string_view>

#include "base/containers/fixed_flat_set.h"

namespace brave_l10n {

// See https://orpa.princeton.edu/export-controls/sanctioned-countries.

inline constexpr auto kOFACSactionedUnM49Codes =
    base::MakeFixedFlatSet<std::string_view>(
        base::sorted_unique,
        {
            // See https://en.wikipedia.org/wiki/UN_M49.

            "001",  // World which includes sanctioned Belarus, Cuba, Iran,
                    // Moldova, North Korea, Russia, Syria and Ukraine.
            "029",  // Caribbean which includes sanctioned Cuba.
            "030",  // Eastern Asia which includes sanctioned North Korea.
            "034",  // Southern Asia which includes sanctioned Iran.
            "145",  // Western Asia which includes sanctioned Syria.
            "151",  // Eastern Europe (including Northern Asia) which includes
                    // sanctioned Belarus, Moldova, Russia and Ukraine.
        });

}  // namespace brave_l10n

#endif  // BRAVE_COMPONENTS_L10N_COMMON_OFAC_SANCTIONED_UN_M49_CODE_CONSTANTS_H_
