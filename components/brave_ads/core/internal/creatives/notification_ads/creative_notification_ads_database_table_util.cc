/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_table_util.h"

#include "base/check.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_daypart_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "url/gurl.h"

namespace brave_ads::database::table {

CreativeNotificationAdInfo CreativeNotificationAdFromMojomRow(
    const mojom::DBRowInfoPtr& mojom_db_row) {
  CHECK(mojom_db_row);

  CreativeNotificationAdInfo creative_ad;

  creative_ad.creative_instance_id = ColumnString(mojom_db_row, 0);
  creative_ad.creative_set_id = ColumnString(mojom_db_row, 1);
  creative_ad.campaign_id = ColumnString(mojom_db_row, 2);
  creative_ad.metric_type =
      ToMojomNewTabPageAdMetricType(ColumnString(mojom_db_row, 3))
          .value_or(mojom::NewTabPageAdMetricType::kUndefined);
  creative_ad.start_at = ColumnTime(mojom_db_row, 4);
  creative_ad.end_at = ColumnTime(mojom_db_row, 5);
  creative_ad.daily_cap = ColumnInt(mojom_db_row, 6);
  creative_ad.advertiser_id = ColumnString(mojom_db_row, 7);
  creative_ad.priority = ColumnInt(mojom_db_row, 8);
  creative_ad.per_day = ColumnInt(mojom_db_row, 9);
  creative_ad.per_week = ColumnInt(mojom_db_row, 10);
  creative_ad.per_month = ColumnInt(mojom_db_row, 11);
  creative_ad.total_max = ColumnInt(mojom_db_row, 12);
  creative_ad.value = ColumnDouble(mojom_db_row, 13);
  creative_ad.segment = ColumnString(mojom_db_row, 14);
  creative_ad.geo_targets.insert(ColumnString(mojom_db_row, 15));
  creative_ad.target_url = GURL(ColumnString(mojom_db_row, 16));
  creative_ad.title = ColumnString(mojom_db_row, 17);
  creative_ad.body = ColumnString(mojom_db_row, 18);
  creative_ad.pass_through_rate = ColumnDouble(mojom_db_row, 19);

  CreativeDaypartInfo daypart;
  daypart.days_of_week = ColumnString(mojom_db_row, 20);
  daypart.start_minute = ColumnInt(mojom_db_row, 21);
  daypart.end_minute = ColumnInt(mojom_db_row, 22);
  creative_ad.dayparts.insert(daypart);

  return creative_ad;
}

}  // namespace brave_ads::database::table
