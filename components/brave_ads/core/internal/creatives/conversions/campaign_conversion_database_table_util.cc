/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_database_table_util.h"

#include "base/check.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

CampaignConversionInfo CampaignConversionFromMojomRow(
    const mojom::DBRowInfoPtr& mojom_db_row) {
  CHECK(mojom_db_row);
  CHECK_EQ(7U, mojom_db_row->column_values_union.size());

  CampaignConversionInfo campaign_conversion;

  campaign_conversion.id = ColumnString(mojom_db_row, 0);

  // `DBColumnValueUnion` has no NULL variant, so, as with `expire_at` below,
  // "unset" is round-tripped through a sentinel rather than SQL NULL.
  const std::string checkout_url_pattern = ColumnString(mojom_db_row, 1);
  if (!checkout_url_pattern.empty()) {
    campaign_conversion.checkout_url_pattern = checkout_url_pattern;
  }

  campaign_conversion.thank_you_url_pattern = ColumnString(mojom_db_row, 2);

  campaign_conversion.extract_id = ColumnBool(mojom_db_row, 3);
  campaign_conversion.extract_value = ColumnBool(mojom_db_row, 4);
  campaign_conversion.observation_window =
      base::Days(ColumnInt(mojom_db_row, 5));
  const base::Time expire_at = ColumnTime(mojom_db_row, 6);
  if (!expire_at.is_null()) {
    campaign_conversion.expire_at = expire_at;
  }

  return campaign_conversion;
}

}  // namespace brave_ads::database::table
