/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/ad_history_database_table_util.h"

#include "base/check.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_type.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"
#include "url/gurl.h"

namespace brave_ads::database::table {

AdHistoryItemInfo AdHistoryItemFromMojomRow(
    const mojom::DBRowInfoPtr& mojom_db_row) {
  CHECK(mojom_db_row);

  AdHistoryItemInfo ad_history_item;

  ad_history_item.created_at = ColumnTime(mojom_db_row, 0);
  ad_history_item.type = ToMojomAdType(ColumnString(mojom_db_row, 1));
  ad_history_item.confirmation_type =
      ToMojomConfirmationType(ColumnString(mojom_db_row, 2));
  ad_history_item.placement_id = ColumnString(mojom_db_row, 3);
  ad_history_item.creative_instance_id = ColumnString(mojom_db_row, 4);
  ad_history_item.creative_set_id = ColumnString(mojom_db_row, 5);
  ad_history_item.campaign_id = ColumnString(mojom_db_row, 6);
  ad_history_item.advertiser_id = ColumnString(mojom_db_row, 7);
  ad_history_item.segment = ColumnString(mojom_db_row, 8);
  ad_history_item.title = ColumnString(mojom_db_row, 9);
  ad_history_item.description = ColumnString(mojom_db_row, 10);
  ad_history_item.target_url = GURL(ColumnString(mojom_db_row, 11));

  return ad_history_item;
}

}  // namespace brave_ads::database::table
