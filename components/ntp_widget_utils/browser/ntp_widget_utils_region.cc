/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/ntp_widget_utils/browser/ntp_widget_utils_region.h"

#include "components/prefs/pref_service.h"
#include "components/country_codes/country_codes.h"

namespace ntp_widget_utils {

bool IsRegionSupported(PrefService* pref_service,
    const std::vector<std::string>& regions,
    bool allow_list)  {
  bool is_supported = !allow_list;
  const int32_t user_region_id =
      country_codes::GetCountryIDFromPrefs(pref_service);

  for (const auto& region : regions) {
    const int32_t region_id = country_codes::CountryCharsToCountryID(
        region.at(0), region.at(1));
    if (user_region_id == region_id) {
      is_supported = !is_supported;
      break;
    }
  }

  return is_supported;
}

std::string FindLocale(const std::vector<std::string>& list,
                       const std::string& default_locale) {
  const std::string locale =
      brave_l10n::LocaleHelper::GetInstance()->GetLocale();
  const std::string user_locale = brave_l10n::GetLanguageCode(locale);
  if (std::find(list.begin(), list.end(), user_locale) != list.end()) {
    return user_locale;
  } else {
    return default_locale;
  }
}

}  // namespace ntp_widget_utils
