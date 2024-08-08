/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/ad_history_database_table.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/debug/dump_without_crashing.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/containers/container_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_bind_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/history/ad_history_feature.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "ad_history";

constexpr int kDefaultBatchSize = 50;

void BindRecords(mojom::DBCommandInfo* const command) {
  CHECK(command);

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::INT64_TYPE,   // created_at
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // type
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // confirmation_type
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // placement_id
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // creative_instance_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // campaign_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // advertiser_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // segment
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // title
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // description
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE   // target_url
  };
}

size_t BindParameters(mojom::DBCommandInfo* command,
                      const AdHistoryList& ad_history) {
  CHECK(command);

  size_t count = 0;

  int index = 0;
  for (const auto& ad_history_item : ad_history) {
    if (!ad_history_item.IsValid()) {
      // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
      // potential defects using `DumpWithoutCrashing`.
      SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                                "Invalid ad history item");
      base::debug::DumpWithoutCrashing();

      BLOG(0, "Invalid ad history item");

      continue;
    }

    BindInt64(command, index++,
              ToChromeTimestampFromTime(ad_history_item.created_at));
    BindString(command, index++, ToString(ad_history_item.type));
    BindString(command, index++, ToString(ad_history_item.confirmation_type));
    BindString(command, index++, ad_history_item.placement_id);
    BindString(command, index++, ad_history_item.creative_instance_id);
    BindString(command, index++, ad_history_item.creative_set_id);
    BindString(command, index++, ad_history_item.campaign_id);
    BindString(command, index++, ad_history_item.advertiser_id);
    BindString(command, index++, ad_history_item.segment);
    BindString(command, index++, ad_history_item.title);
    BindString(command, index++, ad_history_item.description);
    BindString(command, index++, ad_history_item.target_url.spec());

    ++count;
  }

  return count;
}

AdHistoryItemInfo GetFromRecord(mojom::DBRecordInfo* const record) {
  CHECK(record);

  AdHistoryItemInfo ad_history_item;

  ad_history_item.created_at =
      ToTimeFromChromeTimestamp(ColumnInt64(record, 0));
  ad_history_item.type = ToAdType(ColumnString(record, 1));
  ad_history_item.confirmation_type =
      ToConfirmationType(ColumnString(record, 2));
  ad_history_item.placement_id = ColumnString(record, 3);
  ad_history_item.creative_instance_id = ColumnString(record, 4);
  ad_history_item.creative_set_id = ColumnString(record, 5);
  ad_history_item.campaign_id = ColumnString(record, 6);
  ad_history_item.advertiser_id = ColumnString(record, 7);
  ad_history_item.segment = ColumnString(record, 8);
  ad_history_item.title = ColumnString(record, 9);
  ad_history_item.description = ColumnString(record, 10);
  ad_history_item.target_url = GURL(ColumnString(record, 11));

  return ad_history_item;
}

void GetCallback(GetAdHistoryCallback callback,
                 mojom::DBCommandResponseInfoPtr command_response) {
  if (!command_response ||
      command_response->status !=
          mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get ad history");

    return std::move(callback).Run(/*ad_history=*/std::nullopt);
  }

  CHECK(command_response->result);

  AdHistoryList ad_history;

  for (const auto& record : command_response->result->get_records()) {
    const AdHistoryItemInfo ad_history_item = GetFromRecord(&*record);
    if (!ad_history_item.IsValid()) {
      // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
      // potential defects using `DumpWithoutCrashing`.
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

void MigrateToV42(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
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
          );)";
  transaction->commands.push_back(std::move(command));

  // Optimize database query for `GetForDateRange`,
  // `GetHighestRankedPlacementsForDateRange`, and `PurgeExpired`.
  CreateTableIndex(transaction,
                   /*table_name=*/"ad_history", /*columns=*/{"created_at"});

  // Optimize database query for `GetHighestRankedPlacementsForDateRange`.
  CreateTableIndex(transaction, /*table_name=*/"ad_history",
                   /*columns=*/{"confirmation_type"});

  // Optimize database query for `GetHighestRankedPlacementsForDateRange`.
  CreateTableIndex(transaction, /*table_name=*/"ad_history",
                   /*columns=*/{"placement_id"});

  // Optimize database query for `GetForCreativeInstanceId`.
  CreateTableIndex(transaction, /*table_name=*/"ad_history",
                   /*columns=*/{"creative_instance_id"});
}

}  // namespace

AdHistory::AdHistory() : batch_size_(kDefaultBatchSize) {}

void AdHistory::Save(const AdHistoryList& ad_history,
                     ResultCallback callback) const {
  if (ad_history.empty()) {
    return std::move(callback).Run(/*success=*/true);
  }

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  const std::vector<AdHistoryList> batches =
      SplitVector(ad_history, batch_size_);

  for (const auto& batch : batches) {
    InsertOrUpdate(&*transaction, batch);
  }

  RunTransaction(std::move(transaction), std::move(callback));
}

void AdHistory::GetForDateRange(const base::Time from_time,
                                const base::Time to_time,
                                GetAdHistoryCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
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
      {GetTableName(),
       base::NumberToString(ToChromeTimestampFromTime(from_time)),
       base::NumberToString(ToChromeTimestampFromTime(to_time))},
      nullptr);
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  RunDBTransaction(std::move(transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void AdHistory::GetHighestRankedPlacementsForDateRange(
    const base::Time from_time,
    const base::Time to_time,
    GetAdHistoryCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

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
  //      ) as ad_history
  //      WHERE
  //        row_number = 1
  //    )
  //
  // See `src/third_party/sqlite/sqlite_chromium_configuration_flags.gni`.

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
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
                ELSE 4
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
      {GetTableName(),
       base::NumberToString(ToChromeTimestampFromTime(from_time)),
       base::NumberToString(ToChromeTimestampFromTime(to_time))},
      nullptr);
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  RunDBTransaction(std::move(transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void AdHistory::GetForCreativeInstanceId(
    const std::string& creative_instance_id,
    GetAdHistoryCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
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
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  RunDBTransaction(std::move(transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void AdHistory::PurgeExpired(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql = base::ReplaceStringPlaceholders(
      R"(
          DELETE FROM
            $1
          WHERE
            created_at <= $2;)",
      {GetTableName(),
       base::NumberToString(ToChromeTimestampFromTime(
           base::Time::Now() - kAdHistoryRetentionPeriod.Get()))},
      nullptr);
  transaction->commands.push_back(std::move(command));

  RunTransaction(std::move(transaction), std::move(callback));
}

std::string AdHistory::GetTableName() const {
  return kTableName;
}

void AdHistory::Create(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
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
          );)";
  transaction->commands.push_back(std::move(command));

  // Optimize database query for `GetForDateRange`,
  // `GetHighestRankedPlacementsForDateRange`, and `PurgeExpired`.
  CreateTableIndex(transaction, GetTableName(), /*columns=*/{"created_at"});

  // Optimize database query for `GetHighestRankedPlacementsForDateRange`.
  CreateTableIndex(transaction, GetTableName(),
                   /*columns=*/{"confirmation_type"});

  // Optimize database query for `GetHighestRankedPlacementsForDateRange`.
  CreateTableIndex(transaction, GetTableName(), /*columns=*/{"placement_id"});

  // Optimize database query for `GetForCreativeInstanceId`.
  CreateTableIndex(transaction, GetTableName(),
                   /*columns=*/{"creative_instance_id"});
}

void AdHistory::Migrate(mojom::DBTransactionInfo* const transaction,
                        const int to_version) {
  CHECK(transaction);

  switch (to_version) {
    case 42: {
      MigrateToV42(transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void AdHistory::InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                               const AdHistoryList& ad_history) const {
  CHECK(transaction);

  if (ad_history.empty()) {
    return;
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->sql = BuildInsertOrUpdateSql(&*command, ad_history);
  transaction->commands.push_back(std::move(command));
}

std::string AdHistory::BuildInsertOrUpdateSql(
    mojom::DBCommandInfo* command,
    const AdHistoryList& ad_history) const {
  CHECK(command);

  const size_t binded_parameters_count = BindParameters(command, ad_history);

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
      {GetTableName(), BuildBindingParameterPlaceholders(
                           /*parameters_count=*/12, binded_parameters_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
