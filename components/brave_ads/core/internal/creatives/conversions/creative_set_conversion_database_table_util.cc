/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table_util.h"

#include "base/check.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

CreativeSetConversionInfo CreativeSetConversionFromMojomRow(
    const mojom::DBRowInfoPtr& mojom_db_row) {
  CHECK(mojom_db_row);

  CreativeSetConversionInfo creative_set_conversion;

  creative_set_conversion.id = ColumnString(mojom_db_row, 0);
  creative_set_conversion.url_pattern = ColumnString(mojom_db_row, 1);
  creative_set_conversion.observation_window =
      base::Days(ColumnInt(mojom_db_row, 2));
  const base::Time expire_at = ColumnTime(mojom_db_row, 3);
  if (!expire_at.is_null()) {
    creative_set_conversion.expire_at = expire_at;
  }

  return creative_set_conversion;
}

}  // namespace brave_ads::database::table
