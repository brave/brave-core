/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/deposits/deposit_util.h"

#include <string_view>
#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

namespace {

constexpr std::string_view kFirstTimeKey = "firstTime";

void CheckIfFirstTimeAndDepositWithUserDataCallback(
    mojom::AdType ad_type,
    mojom::ConfirmationType confirmation_type,
    const std::string& creative_instance_id,
    const std::string& segment,
    base::Value::Dict user_data,
    bool success,
    bool is_first_time) {
  if (!success) {
    return BLOG(0, "Failed to deposit");
  }

  if (is_first_time) {
    user_data.Set(kFirstTimeKey, true);
  }

  GetAccount().DepositWithUserData(creative_instance_id, segment, ad_type,
                                   confirmation_type, std::move(user_data));
}

void CheckIfFirstTimeAndDepositWithUserData(
    mojom::AdType ad_type,
    mojom::ConfirmationType confirmation_type,
    const std::string& campaign_id,
    const std::string& creative_instance_id,
    const std::string& segment,
    base::Value::Dict user_data) {
  database::table::AdEvents ad_events_database_table;
  ad_events_database_table.IsFirstTime(
      campaign_id, confirmation_type,
      base::BindOnce(&CheckIfFirstTimeAndDepositWithUserDataCallback, ad_type,
                     confirmation_type, creative_instance_id, segment,
                     std::move(user_data)));
}

}  // namespace

void DepositWithUserData(mojom::AdType ad_type,
                         mojom::ConfirmationType confirmation_type,
                         const std::string& campaign_id,
                         const std::string& creative_instance_id,
                         const std::string& segment,
                         base::Value::Dict user_data) {
  if (confirmation_type != mojom::ConfirmationType::kViewedImpression) {
    return GetAccount().DepositWithUserData(creative_instance_id, segment,
                                            ad_type, confirmation_type,
                                            std::move(user_data));
  }

  CheckIfFirstTimeAndDepositWithUserData(ad_type, confirmation_type,
                                         campaign_id, creative_instance_id,
                                         segment, std::move(user_data));
}

void Deposit(mojom::AdType ad_type,
             mojom::ConfirmationType confirmation_type,
             const std::string& campaign_id,
             const std::string& creative_instance_id,
             const std::string& segment) {
  DepositWithUserData(ad_type, confirmation_type, campaign_id,
                      creative_instance_id, segment,
                      /*user_data=*/base::Value::Dict());
}

}  // namespace brave_ads
