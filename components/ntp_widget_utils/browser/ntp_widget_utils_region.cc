/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "base/containers/contains.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/ntp_widget_utils/browser/ntp_widget_utils_region.h"
#include "components/country_codes/country_codes.h"
#include "components/prefs/pref_service.h"

namespace ntp_widget_utils {

bool IsRegionSupported(PrefService* pref_service,
    const std::vector<std::string>& regions,
    bool allow_list)  {
  bool is_supported = !allow_list;
  const country_codes::CountryId user_region_id =
      country_codes::GetCountryIDFromPrefs(pref_service);

  for (const auto& region : regions) {
    auto region_id = country_codes::CountryId(region);
    if (user_region_id == region_id) {
      is_supported = !is_supported;
      break;
    }
  }

  return is_supported;
}

std::string FindLocale(const std::vector<std::string>& list,
                       const std::string& default_locale) {
  const std::string language_code =
      brave_l10n::GetDefaultISOLanguageCodeString();
  if (!base::Contains(list, language_code)) {
    return default_locale;
  }

  return language_code;
}

}  // namespace ntp_widget_utils
