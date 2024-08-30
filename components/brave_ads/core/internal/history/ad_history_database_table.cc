/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/ad_history_database_table.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/debug/dump_without_crashing.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/containers/container_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_statement_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "brave/components/brave_ads/core/public/history/ad_history_feature.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "ad_history";

constexpr int kDefaultBatchSize = 50;

void BindColumnTypes(mojom::DBActionInfo* const mojom_db_action) {
  CHECK(mojom_db_action);

  mojom_db_action->bind_column_types = {
      mojom::DBBindColumnType::kTime,    // created_at
      mojom::DBBindColumnType::kString,  // type
      mojom::DBBindColumnType::kString,  // confirmation_type
      mojom::DBBindColumnType::kString,  // placement_id
      mojom::DBBindColumnType::kString,  // creative_instance_id
      mojom::DBBindColumnType::kString,  // creative_set_id
      mojom::DBBindColumnType::kString,  // campaign_id
      mojom::DBBindColumnType::kString,  // advertiser_id
      mojom::DBBindColumnType::kString,  // segment
      mojom::DBBindColumnType::kString,  // title
      mojom::DBBindColumnType::kString,  // description
      mojom::DBBindColumnType::kString   // target_url
  };
}

size_t BindColumns(mojom::DBActionInfo* mojom_db_action,
                   const AdHistoryList& ad_history) {
  CHECK(mojom_db_action);
  CHECK(!ad_history.empty());

  size_t row_count = 0;

  int index = 0;
  for (const auto& ad_history_item : ad_history) {
    if (!ad_history_item.IsValid()) {
      // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
      // potential defects using `DumpWithoutCrashing`.
      SCOPED_CRASH_KEY_STRING64("Issue32066", "ad_type",
                                ToString(ad_history_item.type));
      SCOPED_CRASH_KEY_STRING64("Issue32066", "confirmation_type",
                                ToString(ad_history_item.confirmation_type));
      SCOPED_CRASH_KEY_STRING64("Issue32066", "creative_instance_id",
                                ad_history_item.creative_instance_id);
      SCOPED_CRASH_KEY_STRING64("Issue32066", "advertiser_id",
                                ad_history_item.advertiser_id);
      SCOPED_CRASH_KEY_STRING64("Issue32066", "segment",
                                ad_history_item.segment);
      SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                                "Invalid ad history item");
      base::debug::DumpWithoutCrashing();

      BLOG(0, "Invalid ad history item");

      continue;
    }

    BindColumnTime(mojom_db_action, index++, ad_history_item.created_at);
    BindColumnString(mojom_db_action, index++, ToString(ad_history_item.type));
    BindColumnString(mojom_db_action, index++,
                     ToString(ad_history_item.confirmation_type));
    BindColumnString(mojom_db_action, index++, ad_history_item.placement_id);
    BindColumnString(mojom_db_action, index++,
                     ad_history_item.creative_instance_id);
    BindColumnString(mojom_db_action, index++, ad_history_item.creative_set_id);
    BindColumnString(mojom_db_action, index++, ad_history_item.campaign_id);
    BindColumnString(mojom_db_action, index++, ad_history_item.advertiser_id);
    BindColumnString(mojom_db_action, index++, ad_history_item.segment);
    BindColumnString(mojom_db_action, index++, ad_history_item.title);
    BindColumnString(mojom_db_action, index++, ad_history_item.description);
    BindColumnString(mojom_db_action, index++,
                     ad_history_item.target_url.spec());

    ++row_count;
  }

  return row_count;
}

AdHistoryItemInfo FromMojomRow(const mojom::DBRowInfo* const mojom_db_row) {
  CHECK(mojom_db_row);

  AdHistoryItemInfo ad_history_item;

  ad_history_item.created_at = ColumnTime(mojom_db_row, 0);
  ad_history_item.type = ToAdType(ColumnString(mojom_db_row, 1));
  ad_history_item.confirmation_type =
      ToConfirmationType(ColumnString(mojom_db_row, 2));
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

void GetCallback(
    GetAdHistoryCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (IsError(&*mojom_db_transaction_result)) {
    BLOG(0, "Failed to get ad history");

    return std::move(callback).Run(/*ad_history=*/std::nullopt);
  }

  CHECK(mojom_db_transaction_result->rows_union);

  AdHistoryList ad_history;

  for (const auto& mojom_db_row :
       mojom_db_transaction_result->rows_union->get_rows()) {
    const AdHistoryItemInfo ad_history_item = FromMojomRow(&*mojom_db_row);
    if (!ad_history_item.IsValid()) {
      // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
      // potential defects using `DumpWithoutCrashing`.
      SCOPED_CRASH_KEY_STRING64("Issue32066", "ad_type",
                                ToString(ad_history_item.type));
      SCOPED_CRASH_KEY_STRING64("Issue32066", "confirmation_type",
                                ToString(ad_history_item.confirmation_type));
      SCOPED_CRASH_KEY_STRING64("Issue32066", "creative_instance_id",
                                ad_history_item.creative_instance_id);
      SCOPED_CRASH_KEY_STRING64("Issue32066", "advertiser_id",
                                ad_history_item.advertiser_id);
      SCOPED_CRASH_KEY_STRING64("Issue32066", "segment",
                                ad_history_item.segment);
      SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                                "Invalid ad history item");
      base::debug::DumpWithoutCrashing();

      BLOG(0, "Invalid ad history item");

      continue;
    }

    ad_history.push_back(ad_history_item);
  }

  std::move(callback).Run(ad_history);
}

void MigrateToV42(mojom::DBTransactionInfo* const mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE ad_history (
        id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
        created_at TIMESTAMP NOT NULL,
        type TEXT NOT NULL,
        confirmation_type TEXT NOT NULL,
        placement_id TEXT NOT NULL,
        creative_instance_id TEXT NOT NULL,
        creative_set_id TEXT NOT NULL,
        campaign_id TEXT NOT NULL,
        advertiser_id TEXT NOT NULL,
        segment TEXT NOT NULL,
        title TEXT NOT NULL,
        description TEXT NOT NULL,
        target_url TEXT NOT NULL
      );)");

  // Optimize database query for `GetForDateRange`,
  // `GetHighestRankedPlacementsForDateRange`, and `PurgeExpired`.
  CreateTableIndex(mojom_db_transaction,
                   /*table_name=*/"ad_history", /*columns=*/{"created_at"});

  // Optimize database query for `GetHighestRankedPlacementsForDateRange`.
  CreateTableIndex(mojom_db_transaction, /*table_name=*/"ad_history",
                   /*columns=*/{"confirmation_type"});
  CreateTableIndex(mojom_db_transaction, /*table_name=*/"ad_history",
                   /*columns=*/{"placement_id"});

  // Optimize database query for `GetForCreativeInstanceId`.
  CreateTableIndex(mojom_db_transaction, /*table_name=*/"ad_history",
                   /*columns=*/{"creative_instance_id"});
}

}  // namespace

AdHistory::AdHistory() : batch_size_(kDefaultBatchSize) {}

void AdHistory::Save(const AdHistoryList& ad_history,
                     ResultCallback callback) const {
  if (ad_history.empty()) {
    return std::move(callback).Run(/*success=*/true);
  }

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  const std::vector<AdHistoryList> batches =
      SplitVector(ad_history, batch_size_);

  for (const auto& batch : batches) {
    Insert(&*mojom_db_transaction, batch);
  }

  RunDBTransaction(std::move(mojom_db_transaction), std::move(callback));
}

void AdHistory::GetForDateRange(const base::Time from_time,
                                const base::Time to_time,
                                GetAdHistoryCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kStepStatement;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            created_at,
            type,
            confirmation_type,
            placement_id,
            creative_instance_id,
            creative_set_id,
            campaign_id,
            advertiser_id,
            segment,
            title,
            description,
            target_url
          FROM
            $1
          WHERE
            created_at BETWEEN $2 AND $3
          ORDER BY
            created_at DESC;)",
      {GetTableName(), TimeToSqlValueAsString(from_time),
       TimeToSqlValueAsString(to_time)},
      nullptr);
  BindColumnTypes(&*mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  GetAdsClient()->RunDBTransaction(
      std::move(mojom_db_transaction),
      base::BindOnce(&GetCallback, std::move(callback)));
}

void AdHistory::GetHighestRankedPlacementsForDateRange(
    const base::Time from_time,
    const base::Time to_time,
    GetAdHistoryCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  // Chrome doesn't use window functions in SQL so we are unable to use:
  //
  //    FilteredAdHistory AS (
  //      SELECT
  //        *
  //      FROM (
  //        SELECT
  //          *,
  //          ROW_NUMBER() OVER (
  //            PARTITION BY
  //             placement_id
  //           ORDER BY
  //              priority
  //          ) as row_number
  //        FROM
  //          PrioritizedAdHistory
  //      ) as filtered_ad_history
  //      WHERE
  //        row_number = 1
  //    )
  //
  // See `src/third_party/sqlite/sqlite_chromium_configuration_flags.gni`.

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kStepStatement;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          -- This query uses a common table expression (CTE) to assign a
          -- numerical priority to each `confirmation_type` within the
          -- `created_at` date range.

          WITH PrioritizedAdHistory AS (
            SELECT
              *,
              CASE confirmation_type
                WHEN 'click' THEN 1
                WHEN 'dismiss' THEN 2
                WHEN 'view' THEN 3
                ELSE 0
              END AS priority
            FROM
              $1
            WHERE
              created_at BETWEEN $2 AND $3
          ),

          -- Then, it uses another CTE to filter the records, keeping only the
          -- one with the lowest priority for each `placement_id`.

          FilteredAdHistory AS (
            SELECT
              *
            FROM
              PrioritizedAdHistory as ad_history
            WHERE
              priority = (
                SELECT
                  MIN(priority)
                FROM
                  PrioritizedAdHistory AS other_ad_history
                WHERE
                  other_ad_history.placement_id = ad_history.placement_id
                  AND other_ad_history.priority > 0
              )
          )

          -- Finally, it selects the required columns from the filtered records
          -- and returns them sorted in descending order by `created_at`.

          SELECT
            created_at,
            type,
            confirmation_type,
            placement_id,
            creative_instance_id,
            creative_set_id,
            campaign_id,
            advertiser_id,
            segment,
            title,
            description,
            target_url
          FROM
            FilteredAdHistory
          ORDER BY
            created_at DESC;)",
      {GetTableName(), TimeToSqlValueAsString(from_time),
       TimeToSqlValueAsString(to_time)},
      nullptr);
  BindColumnTypes(&*mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  GetAdsClient()->RunDBTransaction(
      std::move(mojom_db_transaction),
      base::BindOnce(&GetCallback, std::move(callback)));
}

void AdHistory::GetForCreativeInstanceId(
    const std::string& creative_instance_id,
    GetAdHistoryCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kStepStatement;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            created_at,
            type,
            confirmation_type,
            placement_id,
            creative_instance_id,
            creative_set_id,
            campaign_id,
            advertiser_id,
            segment,
            title,
            description,
            target_url
          FROM
            $1
          WHERE
            creative_instance_id = '$2';)",
      {GetTableName(), creative_instance_id}, nullptr);
  BindColumnTypes(&*mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  GetAdsClient()->RunDBTransaction(
      std::move(mojom_db_transaction),
      base::BindOnce(&GetCallback, std::move(callback)));
}

void AdHistory::PurgeExpired(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  Execute(&*mojom_db_transaction, R"(
            DELETE FROM
              $1
            WHERE
              created_at <= $2;)",
          {GetTableName(),
           TimeToSqlValueAsString(base::Time::Now() -
                                  kAdHistoryRetentionPeriod.Get())});

  RunDBTransaction(std::move(mojom_db_transaction), std::move(callback));
}

std::string AdHistory::GetTableName() const {
  return kTableName;
}

void AdHistory::Create(mojom::DBTransactionInfo* const mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE ad_history (
        id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
        created_at TIMESTAMP NOT NULL,
        type TEXT NOT NULL,
        confirmation_type TEXT NOT NULL,
        placement_id TEXT NOT NULL,
        creative_instance_id TEXT NOT NULL,
        creative_set_id TEXT NOT NULL,
        campaign_id TEXT NOT NULL,
        advertiser_id TEXT NOT NULL,
        segment TEXT NOT NULL,
        title TEXT NOT NULL,
        description TEXT NOT NULL,
        target_url TEXT NOT NULL
      );)");

  // Optimize database query for `GetForDateRange`,
  // `GetHighestRankedPlacementsForDateRange`, and `PurgeExpired` from
  // schema 42.
  CreateTableIndex(mojom_db_transaction, GetTableName(),
                   /*columns=*/{"created_at"});

  // Optimize database query for `GetHighestRankedPlacementsForDateRange` from
  // schema 42.
  CreateTableIndex(mojom_db_transaction, GetTableName(),
                   /*columns=*/{"confirmation_type"});
  CreateTableIndex(mojom_db_transaction, GetTableName(),
                   /*columns=*/{"placement_id"});

  // Optimize database query for `GetForCreativeInstanceId` from schema 42.
  CreateTableIndex(mojom_db_transaction, GetTableName(),
                   /*columns=*/{"creative_instance_id"});
}

void AdHistory::Migrate(mojom::DBTransactionInfo* const mojom_db_transaction,
                        const int to_version) {
  CHECK(mojom_db_transaction);

  switch (to_version) {
    case 42: {
      MigrateToV42(mojom_db_transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void AdHistory::Insert(mojom::DBTransactionInfo* mojom_db_transaction,
                       const AdHistoryList& ad_history) const {
  CHECK(mojom_db_transaction);

  if (ad_history.empty()) {
    return;
  }

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kRunStatement;
  mojom_db_action->sql = BuildInsertSql(&*mojom_db_action, ad_history);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

std::string AdHistory::BuildInsertSql(mojom::DBActionInfo* mojom_db_action,
                                      const AdHistoryList& ad_history) const {
  CHECK(mojom_db_action);
  CHECK(!ad_history.empty());

  const size_t row_count = BindColumns(mojom_db_action, ad_history);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            created_at,
            type,
            confirmation_type,
            placement_id,
            creative_instance_id,
            creative_set_id,
            campaign_id,
            advertiser_id,
            segment,
            title,
            description,
            target_url
          ) VALUES $2;)",
      {GetTableName(),
       BuildBindColumnPlaceholders(/*column_count=*/12, row_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
