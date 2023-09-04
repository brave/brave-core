/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/locale_user_data.h"

#include <string>

#include "brave/components/brave_ads/core/internal/common/locale/country_code_anonymity_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/l10n/common/locale_util.h"

namespace brave_ads {

namespace {

constexpr char kCountryCodeKey[] = "countryCode";
constexpr char kOtherCountryCode[] = "??";

}  // namespace

base::Value::Dict BuildLocaleUserData() {
  base::Value::Dict user_data;

  if (!UserHasJoinedBraveRewards()) {
    return user_data;
  }

  const auto& build_channel = GlobalState::GetInstance()->BuildChannel();
  if (!build_channel.is_release) {
    return user_data;
  }

  const std::string country_code = brave_l10n::GetDefaultISOCountryCodeString();

  if (IsCountryCodeMemberOfAnonymitySet(country_code)) {
    user_data.Set(kCountryCodeKey, country_code);
  } else {
    if (ShouldClassifyCountryCodeAsOther(country_code)) {
      user_data.Set(kCountryCodeKey, kOtherCountryCode);
    }
  }

  return user_data;
}

}  // namespace brave_ads
