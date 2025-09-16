/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/country_code_user_data.h"

#include <string_view>

namespace brave_ads {

namespace {
constexpr std::string_view kCountryCodeKey = "countryCode";
}  // namespace

base::Value::Dict BuildCountryCodeUserData() {
  const std::string_view country_code = "XX";

  return base::Value::Dict().Set(kCountryCodeKey, country_code);
}

}  // namespace brave_ads
