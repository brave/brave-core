/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table_test_util.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::test {

namespace {

void GetAllCallback(
    database::table::GetAdEventsCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (!database::IsTransactionSuccessful(mojom_db_transaction_result)) {
    BLOG(0, "Failed to get ad events");
    return std::move(callback).Run(/*success=*/false, /*ad_events=*/{});
  }

  CHECK(mojom_db_transaction_result->rows_union);

  AdEventList ad_events;
  for (const auto& mojom_db_row :
       mojom_db_transaction_result->rows_union->get_rows()) {
    const AdEventInfo ad_event =
        database::table::AdEventFromMojomRow(mojom_db_row);
    if (!ad_event.IsValid()) {
      BLOG(0, "Invalid ad event");
      continue;
    }

    ad_events.push_back(ad_event);
  }

  std::move(callback).Run(/*success=*/true, ad_events);
}

}  // namespace

void GetAll(database::table::GetAdEventsCallback callback) {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteQueryWithBindings;
  mojom_db_action->sql = R"(
      SELECT
        placement_id,
        type,
        confirmation_type,
        campaign_id,
        creative_set_id,
        creative_instance_id,
        advertiser_id,
        segment,
        target_url,
        created_at
      FROM
        ad_events)";
  mojom_db_action->bind_column_types = {
      mojom::DBBindColumnType::kString,  // placement_id
      mojom::DBBindColumnType::kString,  // type
      mojom::DBBindColumnType::kString,  // confirmation type
      mojom::DBBindColumnType::kString,  // campaign_id
      mojom::DBBindColumnType::kString,  // creative_set_id
      mojom::DBBindColumnType::kString,  // creative_instance_id
      mojom::DBBindColumnType::kString,  // advertiser_id
      mojom::DBBindColumnType::kString,  // segment
      mojom::DBBindColumnType::kString,  // target_url
      mojom::DBBindColumnType::kTime     // created_at
  };
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  database::RunTransaction(
      FROM_HERE, std::move(mojom_db_transaction),
      base::BindOnce(&GetAllCallback, std::move(callback)));
}

}  // namespace brave_ads::test
