/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/locale_user_data.h"

#include <string>

#include "base/values.h"
#include "bat/ads/ads.h"
#include "bat/ads/internal/locale/country_code_util.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"

namespace ads {
namespace user_data {

base::DictionaryValue GetLocale() {
  base::DictionaryValue user_data;

  if (!BuildChannel().is_release) {
    return user_data;
  }

  const std::string locale =
      brave_l10n::LocaleHelper::GetInstance()->GetLocale();

  if (locale::IsMemberOfAnonymitySet(locale)) {
    const std::string country_code = brave_l10n::GetCountryCode(locale);
    user_data.SetStringKey("countryCode", country_code);
  } else {
    if (locale::ShouldClassifyAsOther(locale)) {
      user_data.SetStringKey("countryCode", "??");
    }
  }

  return user_data;
}

}  // namespace user_data
}  // namespace ads
