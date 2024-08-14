/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"

#include <cstddef>
#include <utility>

#include "base/check.h"
#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_statement_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "ad_events";

void BindColumnTypes(mojom::DBStatementInfo* const mojom_statement) {
  CHECK(mojom_statement);

  mojom_statement->bind_column_types = {
      mojom::DBBindColumnType::kString,  // placement_id
      mojom::DBBindColumnType::kString,  // type
      mojom::DBBindColumnType::kString,  // confirmation type
      mojom::DBBindColumnType::kString,  // campaign_id
      mojom::DBBindColumnType::kString,  // creative_set_id
      mojom::DBBindColumnType::kString,  // creative_instance_id
      mojom::DBBindColumnType::kString,  // advertiser_id
      mojom::DBBindColumnType::kString,  // segment
      mojom::DBBindColumnType::kTime     // created_at
  };
}

size_t BindColumns(mojom::DBStatementInfo* mojom_statement,
                   const AdEventList& ad_events) {
  CHECK(mojom_statement);
  CHECK(!ad_events.empty());

  size_t row_count = 0;

  int index = 0;
  for (const auto& ad_event : ad_events) {
    if (!ad_event.IsValid()) {
      // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
      // potential defects using `DumpWithoutCrashing`.
      SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                                "Invalid ad event");
      base::debug::DumpWithoutCrashing();

      BLOG(0, "Invalid ad event");

      continue;
    }

    BindColumnString(mojom_statement, index++, ad_event.placement_id);
    BindColumnString(mojom_statement, index++, ToString(ad_event.type));
    BindColumnString(mojom_statement, index++,
                     ToString(ad_event.confirmation_type));
    BindColumnString(mojom_statement, index++, ad_event.campaign_id);
    BindColumnString(mojom_statement, index++, ad_event.creative_set_id);
    BindColumnString(mojom_statement, index++, ad_event.creative_instance_id);
    BindColumnString(mojom_statement, index++, ad_event.advertiser_id);
    BindColumnString(mojom_statement, index++, ad_event.segment);
    BindColumnTime(mojom_statement, index++,
                   ad_event.created_at.value_or(base::Time()));

    ++row_count;
  }

  return row_count;
}

AdEventInfo FromMojomRow(const mojom::DBRowInfo* const mojom_row) {
  CHECK(mojom_row);

  AdEventInfo ad_event;

  ad_event.placement_id = ColumnString(mojom_row, 0);
  ad_event.type = ToAdType(ColumnString(mojom_row, 1));
  ad_event.confirmation_type = ToConfirmationType(ColumnString(mojom_row, 2));
  ad_event.campaign_id = ColumnString(mojom_row, 3);
  ad_event.creative_set_id = ColumnString(mojom_row, 4);
  ad_event.creative_instance_id = ColumnString(mojom_row, 5);
  ad_event.advertiser_id = ColumnString(mojom_row, 6);
  ad_event.segment = ColumnString(mojom_row, 7);
  const base::Time created_at = ColumnTime(mojom_row, 8);
  if (!created_at.is_null()) {
    ad_event.created_at = created_at;
  }

  return ad_event;
}

void GetCallback(GetAdEventsCallback callback,
                 mojom::DBStatementResultInfoPtr mojom_statement_result) {
  if (!mojom_statement_result ||
      mojom_statement_result->result_code !=
          mojom::DBStatementResultInfo::ResultCode::kSuccess) {
    BLOG(0, "Failed to get ad events");

    return std::move(callback).Run(/*success=*/false, /*ad_events=*/{});
  }

  CHECK(mojom_statement_result->rows_union);

  AdEventList ad_events;
  for (const auto& mojom_row : mojom_statement_result->rows_union->get_rows()) {
    const AdEventInfo ad_event = FromMojomRow(&*mojom_row);
    if (!ad_event.IsValid()) {
      // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
      // potential defects using `DumpWithoutCrashing`.
      SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                                "Invalid ad event");
      base::debug::DumpWithoutCrashing();

      BLOG(0, "Invalid ad event");

      continue;
    }

    ad_events.push_back(ad_event);
  }

  std::move(callback).Run(/*success=*/true, ad_events);
}

void MigrateToV5(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Recreate table to address a migration problem from older versions.
  DropTable(mojom_transaction, "ad_events");

  Execute(mojom_transaction, R"(
      CREATE TABLE ad_events (
        id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
        uuid TEXT NOT NULL,
        type TEXT,
        confirmation_type TEXT,
        campaign_id TEXT NOT NULL,
        creative_set_id TEXT NOT NULL,
        creative_instance_id TEXT NOT NULL,
        timestamp TIMESTAMP NOT NULL
      );)");
}

void MigrateToV13(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Create a temporary table:
  //   - with a new `advertiser_id` column.
  //   - with a new `segment` column.
  Execute(mojom_transaction, R"(
      CREATE TABLE ad_events_temp (
        id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
        uuid TEXT NOT NULL,
        type TEXT,
        confirmation_type TEXT,
        campaign_id TEXT NOT NULL,
        creative_set_id TEXT NOT NULL,
        creative_instance_id TEXT NOT NULL,
        advertiser_id TEXT,
        segment TEXT,
        timestamp TIMESTAMP NOT NULL
      );)");

  // Copy legacy columns to the temporary table, drop the legacy table, and
  // rename the temporary table.
  const std::vector<std::string> columns = {"uuid",
                                            "type",
                                            "confirmation_type",
                                            "campaign_id",
                                            "creative_set_id",
                                            "creative_instance_id",
                                            "timestamp"};

  CopyTableColumns(mojom_transaction, "ad_events", "ad_events_temp", columns,
                   /*should_drop=*/true);

  RenameTable(mojom_transaction, "ad_events_temp", "ad_events");
}

void MigrateToV17(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  CreateTableIndex(mojom_transaction, /*table_name=*/"ad_events",
                   /*columns=*/{"timestamp"});
}

void MigrateToV28(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Create a temporary table:
  //   - with a new `segment` column.
  //   - renaming the `timestamp` column to `created_at`.
  Execute(mojom_transaction, R"(
      CREATE TABLE ad_events_temp (
        id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
        placement_id TEXT NOT NULL,
        type TEXT,
        confirmation_type TEXT,
        campaign_id TEXT NOT NULL,
        creative_set_id TEXT NOT NULL,
        creative_instance_id TEXT NOT NULL,
        advertiser_id TEXT,
        segment TEXT,
        created_at TIMESTAMP NOT NULL
      );)");

  // Copy legacy columns to the temporary table, drop the legacy table, rename
  // the temporary table and create an index.
  const std::vector<std::string> from_columns = {"uuid",
                                                 "type",
                                                 "confirmation_type",
                                                 "campaign_id",
                                                 "creative_set_id",
                                                 "creative_instance_id",
                                                 "advertiser_id",
                                                 "timestamp"};

  const std::vector<std::string> to_columns = {
      "placement_id",      "type",
      "confirmation_type", "campaign_id",
      "creative_set_id",   "creative_instance_id",
      "advertiser_id",     "created_at"};

  CopyTableColumns(mojom_transaction, "ad_events", "ad_events_temp",
                   from_columns, to_columns, /*should_drop=*/true);

  RenameTable(mojom_transaction, "ad_events_temp", "ad_events");

  CreateTableIndex(mojom_transaction, /*table_name=*/"ad_events",
                   /*columns=*/{"created_at"});
}

void MigrateToV29(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Migrate `created_at` column from a UNIX timestamp to a WebKit/Chrome
  // timestamp.
  Execute(mojom_transaction, R"(
      UPDATE
        ad_events
      SET
        created_at = (
          CAST(created_at AS INT64) + 11644473600
        ) * 1000000;)");
}

void MigrateToV32(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Migrate `confirmation_type` from 'saved' to 'bookmark'.
  Execute(mojom_transaction, R"(
      UPDATE
        ad_events
      SET
        confirmation_type = 'bookmark'
      WHERE
        confirmation_type == 'saved';)");
}

void MigrateToV35(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  DropTableIndex(mojom_transaction, "ad_events_created_at_index");

  // Optimize database query for `GetUnexpired`.
  CreateTableIndex(mojom_transaction, /*table_name=*/"ad_events",
                   /*columns=*/{"created_at"});
  CreateTableIndex(mojom_transaction, /*table_name=*/"ad_events",
                   /*columns=*/{"type", "created_at"});
}

void MigrateToV41(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Remove non-clicked search result ad events for users who have not joined
  // Brave Rewards.
  if (!UserHasJoinedBraveRewards()) {
    Execute(mojom_transaction, R"(
        DELETE FROM
          ad_events
        WHERE
          type == 'search_result_ad'
          AND confirmation_type != 'click';)");
  }
}

void MigrateToV43(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  DropTableIndex(mojom_transaction, "ad_events_type_creative_set_id_index");

  CreateTableIndex(mojom_transaction, /*table_name=*/"ad_events",
                   /*columns=*/{"type"});
  CreateTableIndex(mojom_transaction, /*table_name=*/"ad_events",
                   /*columns=*/{"confirmation_type"});
  CreateTableIndex(mojom_transaction, /*table_name=*/"ad_events",
                   /*columns=*/{"creative_set_id"});
  CreateTableIndex(mojom_transaction, /*table_name=*/"ad_events",
                   /*columns=*/{"placement_id"});
}

}  // namespace

void AdEvents::RecordEvent(const AdEventInfo& ad_event,
                           ResultCallback callback) {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();

  Insert(&*mojom_transaction, {ad_event});

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

void AdEvents::GetAll(GetAdEventsCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kStep;
  mojom_statement->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            placement_id,
            type,
            confirmation_type,
            campaign_id,
            creative_set_id,
            creative_instance_id,
            advertiser_id,
            segment,
            created_at
          FROM
            $1;)",
      {GetTableName()}, nullptr);
  BindColumnTypes(&*mojom_statement);
  mojom_transaction->statements.push_back(std::move(mojom_statement));

  RunDBTransaction(std::move(mojom_transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void AdEvents::GetUnexpired(GetAdEventsCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kStep;
  mojom_statement->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            placement_id,
            type,
            confirmation_type,
            campaign_id,
            creative_set_id,
            creative_instance_id,
            advertiser_id,
            segment,
            created_at
          FROM
            $1
          WHERE
            creative_set_id IN (
              SELECT
                creative_set_id
              FROM
                creative_set_conversions
            )
            OR created_at > $2
          ORDER BY
            created_at ASC;)",
      {GetTableName(),
       TimeToSqlValueAsString(base::Time::Now() - base::Days(90))},
      nullptr);
  BindColumnTypes(&*mojom_statement);
  mojom_transaction->statements.push_back(std::move(mojom_statement));

  RunDBTransaction(std::move(mojom_transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void AdEvents::GetUnexpired(const mojom::AdType ad_type,
                            GetAdEventsCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kStep;
  mojom_statement->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            placement_id,
            type,
            confirmation_type,
            campaign_id,
            creative_set_id,
            creative_instance_id,
            advertiser_id,
            segment,
            created_at
          FROM
            $1
          WHERE
            type = '$2'
            AND (
              creative_set_id IN (
                SELECT
                  creative_set_id
                FROM
                  creative_set_conversions
              )
              OR created_at > $3
            )
          ORDER BY
            created_at ASC;)",
      {GetTableName(), ToString(static_cast<AdType>(ad_type)),
       TimeToSqlValueAsString(base::Time::Now() - base::Days(90))},
      nullptr);
  BindColumnTypes(&*mojom_statement);
  mojom_transaction->statements.push_back(std::move(mojom_statement));

  RunDBTransaction(std::move(mojom_transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void AdEvents::PurgeExpired(ResultCallback callback) const {
  const size_t days = UserHasJoinedBraveRewards() ? 90 : 30;

  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  Execute(&*mojom_transaction, R"(
            DELETE FROM
              $1
            WHERE
              creative_set_id NOT IN (
                SELECT
                  creative_set_id
                FROM
                  creative_set_conversions
              )
              AND created_at <= $2;)",
          {GetTableName(),
           TimeToSqlValueAsString(base::Time::Now() - base::Days(days))});

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

void AdEvents::PurgeOrphaned(const mojom::AdType ad_type,
                             ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  Execute(
      &*mojom_transaction, R"(
        DELETE FROM
          $1
        WHERE
          placement_id IN (
            SELECT
              placement_id
            FROM
              $2
            GROUP BY
              placement_id
            HAVING
              count(*) = 1
          )
          AND confirmation_type = 'served'
          AND type = '$3';)",
      {GetTableName(), GetTableName(), ToString(static_cast<AdType>(ad_type))});

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

void AdEvents::PurgeOrphaned(const std::vector<std::string>& placement_ids,
                             ResultCallback callback) const {
  if (placement_ids.empty()) {
    return std::move(callback).Run(/*success=*/true);
  }

  std::vector<std::string> quoted_placement_ids;
  quoted_placement_ids.reserve(placement_ids.size());
  for (const auto& placement_id : placement_ids) {
    quoted_placement_ids.push_back(
        base::ReplaceStringPlaceholders("'$1'", {placement_id}, nullptr));
  }

  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  Execute(&*mojom_transaction, R"(
            DELETE FROM
              $1
            WHERE
              placement_id IN (
                SELECT
                  placement_id
                FROM
                  $2
                GROUP BY
                  placement_id
                HAVING
                  count(*) = 1
              )
              AND confirmation_type = 'served'
              AND placement_id IN ($3);)",
          {GetTableName(), GetTableName(),
           base::JoinString(quoted_placement_ids, ", ")});

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

void AdEvents::PurgeAllOrphaned(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  Execute(&*mojom_transaction, R"(
            DELETE FROM
              $1
            WHERE
              placement_id IN (
                SELECT
                  placement_id
                FROM
                  $2
                GROUP BY
                  placement_id
                HAVING
                  count(*) = 1
              )
              AND confirmation_type = 'served';)",
          {GetTableName(), GetTableName()});

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

std::string AdEvents::GetTableName() const {
  return kTableName;
}

void AdEvents::Create(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  Execute(mojom_transaction, R"(
      CREATE TABLE ad_events (
        id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
        placement_id TEXT NOT NULL,
        type TEXT,
        confirmation_type TEXT,
        campaign_id TEXT NOT NULL,
        creative_set_id TEXT NOT NULL,
        creative_instance_id TEXT NOT NULL,
        advertiser_id TEXT,
        segment TEXT,
        created_at TIMESTAMP NOT NULL
      );)");

  // Optimize database query for `GetUnexpired` from schema 35.
  CreateTableIndex(mojom_transaction, GetTableName(),
                   /*columns=*/{"created_at"});

  // Optimize database query for `GetUnexpired` from schema 43.
  CreateTableIndex(mojom_transaction, GetTableName(),
                   /*columns=*/{"type"});
  CreateTableIndex(mojom_transaction, GetTableName(),
                   /*columns=*/{"confirmation_type"});
  CreateTableIndex(mojom_transaction, GetTableName(),
                   /*columns=*/{"creative_set_id"});
  CreateTableIndex(mojom_transaction, GetTableName(),
                   /*columns=*/{"placement_id"});
}

void AdEvents::Migrate(mojom::DBTransactionInfo* mojom_transaction,
                       const int to_version) {
  CHECK(mojom_transaction);

  switch (to_version) {
    case 5: {
      MigrateToV5(mojom_transaction);
      break;
    }

    case 13: {
      MigrateToV13(mojom_transaction);
      break;
    }

    case 17: {
      MigrateToV17(mojom_transaction);
      break;
    }

    case 28: {
      MigrateToV28(mojom_transaction);
      break;
    }

    case 29: {
      MigrateToV29(mojom_transaction);
      break;
    }

    case 32: {
      MigrateToV32(mojom_transaction);
      break;
    }

    case 35: {
      MigrateToV35(mojom_transaction);
      break;
    }

    case 41: {
      MigrateToV41(mojom_transaction);
      break;
    }

    case 43: {
      MigrateToV43(mojom_transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void AdEvents::Insert(mojom::DBTransactionInfo* mojom_transaction,
                      const AdEventList& ad_events) {
  CHECK(mojom_transaction);

  if (ad_events.empty()) {
    return;
  }

  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type = mojom::DBStatementInfo::OperationType::kRun;
  mojom_statement->sql = BuildInsertSql(&*mojom_statement, ad_events);
  mojom_transaction->statements.push_back(std::move(mojom_statement));
}

std::string AdEvents::BuildInsertSql(mojom::DBStatementInfo* mojom_statement,
                                     const AdEventList& ad_events) const {
  CHECK(mojom_statement);
  CHECK(!ad_events.empty());

  const size_t row_count = BindColumns(mojom_statement, ad_events);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            placement_id,
            type,
            confirmation_type,
            campaign_id,
            creative_set_id,
            creative_instance_id,
            advertiser_id,
            segment,
            created_at
          ) VALUES $2;)",
      {GetTableName(),
       BuildBindColumnPlaceholders(/*column_count=*/9, row_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
