/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/summary_user_data.h"

#include <string_view>
#include <utility>

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/summary_user_data_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

namespace brave_ads {

namespace {

constexpr std::string_view kSummaryKey = "totals";
constexpr std::string_view kAdFormatKey = "ad_format";

}  // namespace

base::DictValue BuildSummaryUserData(const PaymentTokenList& payment_tokens) {
  if (!UserHasJoinedBraveRewards()) {
    return {};
  }

  base::ListValue list;
  for (const auto& [mojom_ad_type, confirmations] :
       BuildAdTypeBuckets(payment_tokens)) {
    auto dict = base::DictValue().Set(kAdFormatKey, ToString(mojom_ad_type));

    for (const auto& [confirmation_type, count] : confirmations) {
      dict.Set(ToString(confirmation_type), count);
    }

    list.Append(std::move(dict));
  }

  return base::DictValue().Set(kSummaryKey, std::move(list));
}

}  // namespace brave_ads
