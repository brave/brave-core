/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_L10N_COMMON_COUNTRY_CODE_UTIL_H_
#define BRAVE_COMPONENTS_L10N_COMMON_COUNTRY_CODE_UTIL_H_

#include <string>

class PrefService;

namespace brave_l10n {

// Return the country code from local state pref or default ISO country code.
std::string GetCountryCode(const PrefService* local_state);

}  // namespace brave_l10n

#endif  // BRAVE_COMPONENTS_L10N_COMMON_COUNTRY_CODE_UTIL_H_
