/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/deposits/cash_deposit.h"

#include "bat/ads/internal/database/tables/creative_ads_database_table.h"

namespace ads {

CashDeposit::CashDeposit() = default;

CashDeposit::~CashDeposit() = default;

void CashDeposit::GetValue(const std::string& creative_instance_id,
                           GetDepositCallback callback) {
  database::table::CreativeAds database_table;
  database_table.GetForCreativeInstanceId(
      creative_instance_id,
      [callback](const bool success, const std::string& creative_instance_id,
                 const CreativeAdInfo& creative_ad) {
        if (!success) {
          callback(/* success */ false, /* value */ 0.0);
          return;
        }

        callback(/* success */ true, creative_ad.value);
      });
}

}  // namespace ads
