/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_table_util.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/creatives/condition_matchers_database_table_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "url/gurl.h"

namespace brave_ads::database::table {

CreativeAdInfo CreativeAdFromMojomRow(const mojom::DBRowInfoPtr& mojom_db_row) {
  CHECK(mojom_db_row);

  CreativeAdInfo creative_ad;

  creative_ad.creative_instance_id = ColumnString(mojom_db_row, 0);
  creative_ad.creative_set_id = ColumnString(mojom_db_row, 1);
  creative_ad.per_day = ColumnInt(mojom_db_row, 2);
  creative_ad.per_week = ColumnInt(mojom_db_row, 3);
  creative_ad.per_month = ColumnInt(mojom_db_row, 4);
  creative_ad.total_max = ColumnInt(mojom_db_row, 5);
  creative_ad.value = ColumnDouble(mojom_db_row, 6);
  creative_ad.condition_matchers =
      StringToConditionMatchers(ColumnString(mojom_db_row, 7));
  creative_ad.target_url = GURL(ColumnString(mojom_db_row, 8));

  return creative_ad;
}

}  // namespace brave_ads::database::table
