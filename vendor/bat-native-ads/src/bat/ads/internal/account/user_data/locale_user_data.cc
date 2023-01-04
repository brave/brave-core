/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/locale_user_data.h"

#include <string>

#include "bat/ads/build_channel.h"
#include "bat/ads/internal/privacy/locale/country_code_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "brave/components/l10n/common/locale_util.h"

namespace ads::user_data {

namespace {

constexpr char kCountryCodeKey[] = "countryCode";
constexpr char kOtherCountryCode[] = "??";

}  // namespace

base::Value::Dict GetLocale() {
  base::Value::Dict user_data;

  if (!BuildChannel().is_release) {
    return user_data;
  }

  const std::string country_code = brave_l10n::GetDefaultISOCountryCodeString();

  if (privacy::locale::IsCountryCodeMemberOfAnonymitySet(country_code)) {
    user_data.Set(kCountryCodeKey, country_code);
  } else {
    if (privacy::locale::ShouldClassifyCountryCodeAsOther(country_code)) {
      user_data.Set(kCountryCodeKey, kOtherCountryCode);
    }
  }

  return user_data;
}

}  // namespace ads::user_data
