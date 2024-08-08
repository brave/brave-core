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
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_bind_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "ad_events";

void BindRecords(mojom::DBCommandInfo* const command) {
  CHECK(command);

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // placement_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // type
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // confirmation
                                                             // type
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // campaign_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // creative_instance_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // advertiser_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // segment
      mojom::DBCommandInfo::RecordBindingType::INT64_TYPE    // created_at
  };
}

size_t BindParameters(mojom::DBCommandInfo* command,
                      const AdEventList& ad_events) {
  CHECK(command);

  size_t count = 0;

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

    BindString(command, index++, ad_event.placement_id);
    BindString(command, index++, ToString(ad_event.type));
    BindString(command, index++, ToString(ad_event.confirmation_type));
    BindString(command, index++, ad_event.campaign_id);
    BindString(command, index++, ad_event.creative_set_id);
    BindString(command, index++, ad_event.creative_instance_id);
    BindString(command, index++, ad_event.advertiser_id);
    BindString(command, index++, ad_event.segment);
    BindInt64(
        command, index++,
        ToChromeTimestampFromTime(ad_event.created_at.value_or(base::Time())));

    ++count;
  }

  return count;
}

AdEventInfo GetFromRecord(mojom::DBRecordInfo* const record) {
  CHECK(record);

  AdEventInfo ad_event;

  ad_event.placement_id = ColumnString(record, 0);
  ad_event.type = ToAdType(ColumnString(record, 1));
  ad_event.confirmation_type = ToConfirmationType(ColumnString(record, 2));
  ad_event.campaign_id = ColumnString(record, 3);
  ad_event.creative_set_id = ColumnString(record, 4);
  ad_event.creative_instance_id = ColumnString(record, 5);
  ad_event.advertiser_id = ColumnString(record, 6);
  ad_event.segment = ColumnString(record, 7);
  const base::Time created_at =
      ToTimeFromChromeTimestamp(ColumnInt64(record, 8));
  if (!created_at.is_null()) {
    ad_event.created_at = created_at;
  }

  return ad_event;
}

void GetCallback(GetAdEventsCallback callback,
                 mojom::DBCommandResponseInfoPtr command_response) {
  if (!command_response ||
      command_response->status !=
          mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get ad events");

    return std::move(callback).Run(/*success=*/false, /*ad_events=*/{});
  }

  CHECK(command_response->result);

  AdEventList ad_events;
  for (const auto& record : command_response->result->get_records()) {
    const AdEventInfo ad_event = GetFromRecord(&*record);
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

void MigrateToV5(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  // Recreate table to address a migration problem from older versions.
  DropTable(transaction, "ad_events");

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
          CREATE TABLE ad_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
            uuid TEXT NOT NULL,
            type TEXT,
            confirmation_type TEXT,
            campaign_id TEXT NOT NULL,
            creative_set_id TEXT NOT NULL,
            creative_instance_id TEXT NOT NULL,
            timestamp TIMESTAMP NOT NULL
          );)";
  transaction->commands.push_back(std::move(command));
}

void MigrateToV13(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  // Create a temporary table:
  //   - with a new `advertiser_id` column.
  //   - with a new `segment` column.
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
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
          );)";
  transaction->commands.push_back(std::move(command));

  // Copy legacy columns to the temporary table, drop the legacy table, and
  // rename the temporary table.
  const std::vector<std::string> columns = {"uuid",
                                            "type",
                                            "confirmation_type",
                                            "campaign_id",
                                            "creative_set_id",
                                            "creative_instance_id",
                                            "timestamp"};

  CopyTableColumns(transaction, "ad_events", "ad_events_temp", columns,
                   /*should_drop=*/true);

  RenameTable(transaction, "ad_events_temp", "ad_events");
}

void MigrateToV17(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  CreateTableIndex(transaction, /*table_name=*/"ad_events",
                   /*columns=*/{"timestamp"});
}

void MigrateToV28(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  // Create a temporary table:
  //   - with a new `segment` column.
  //   - renaming the `timestamp` column to `created_at`.
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
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
          );)";
  transaction->commands.push_back(std::move(command));

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

  CopyTableColumns(transaction, "ad_events", "ad_events_temp", from_columns,
                   to_columns,
                   /*should_drop=*/true);

  RenameTable(transaction, "ad_events_temp", "ad_events");

  CreateTableIndex(transaction, /*table_name=*/"ad_events",
                   /*columns=*/{"created_at"});
}

void MigrateToV29(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  // Migrate `created_at` column from a UNIX timestamp to a WebKit/Chrome
  // timestamp.
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
          UPDATE
            ad_events
          SET
            created_at = (
              CAST(created_at AS INT64) + 11644473600
            ) * 1000000;)";
  transaction->commands.push_back(std::move(command));
}

void MigrateToV32(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  // Migrate `confirmation_type` from 'saved' to 'bookmark'.
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
          UPDATE
            ad_events
          SET
            confirmation_type = 'bookmark'
          WHERE
            confirmation_type == 'saved';)";
  transaction->commands.push_back(std::move(command));
}

void MigrateToV35(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  DropTableIndex(transaction, "ad_events_created_at_index");

  // Optimize database query for `GetUnexpired`.
  CreateTableIndex(transaction, /*table_name=*/"ad_events",
                   /*columns=*/{"created_at"});
  CreateTableIndex(transaction, /*table_name=*/"ad_events",
                   /*columns=*/{"type", "created_at"});
}

void MigrateToV41(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  // Remove non-clicked search result ad events for users who have not joined
  // Brave Rewards.
  if (!UserHasJoinedBraveRewards()) {
    mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
    command->type = mojom::DBCommandInfo::Type::EXECUTE;
    command->sql =
        R"(
            DELETE FROM
              ad_events
            WHERE
              type == 'search_result_ad'
              AND confirmation_type != 'click';)";
    transaction->commands.push_back(std::move(command));
  }
}

}  // namespace

void AdEvents::RecordEvent(const AdEventInfo& ad_event,
                           ResultCallback callback) {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  InsertOrUpdate(&*transaction, {ad_event});

  RunTransaction(std::move(transaction), std::move(callback));
}

void AdEvents::GetAll(GetAdEventsCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
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
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  RunDBTransaction(std::move(transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void AdEvents::GetUnexpired(GetAdEventsCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
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
            OR DATETIME(
              (created_at / 1000000) - 11644473600,
              'unixepoch'
            ) > DATETIME(
              ($2 / 1000000) - 11644473600,
              'unixepoch',
              '-3 months'
            )
          ORDER BY
            created_at ASC;)",
      {GetTableName(),
       base::NumberToString(ToChromeTimestampFromTime(base::Time::Now()))},
      nullptr);
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  RunDBTransaction(std::move(transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void AdEvents::GetUnexpired(const mojom::AdType ad_type,
                            GetAdEventsCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
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
              OR DATETIME(
                (created_at / 1000000) - 11644473600,
                'unixepoch'
              ) > DATETIME(
                ($3 / 1000000) - 11644473600,
                'unixepoch',
                '-3 months'
              )
            )
          ORDER BY
            created_at ASC;)",
      {GetTableName(), ToString(static_cast<AdType>(ad_type)),
       base::NumberToString(ToChromeTimestampFromTime(base::Time::Now()))},
      nullptr);
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  RunDBTransaction(std::move(transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void AdEvents::PurgeExpired(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql = base::ReplaceStringPlaceholders(
      R"(
          DELETE FROM
            $1
          WHERE
            creative_set_id NOT IN (
              SELECT
                creative_set_id
              FROM
                creative_set_conversions
            )
            AND DATETIME(
              (created_at / 1000000) - 11644473600,
              'unixepoch'
            ) <= DATETIME(
              ($2 / 1000000) - 11644473600,
              'unixepoch',
              '$3'
            );)",
      {GetTableName(),
       base::NumberToString(ToChromeTimestampFromTime(base::Time::Now())),
       UserHasJoinedBraveRewards() ? "-3 months" : "-30 days"},
      nullptr);
  transaction->commands.push_back(std::move(command));

  RunTransaction(std::move(transaction), std::move(callback));
}

void AdEvents::PurgeOrphaned(const mojom::AdType ad_type,
                             ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql = base::ReplaceStringPlaceholders(
      R"(
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
      {GetTableName(), GetTableName(), ToString(static_cast<AdType>(ad_type))},
      nullptr);
  transaction->commands.push_back(std::move(command));

  RunTransaction(std::move(transaction), std::move(callback));
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

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql = base::ReplaceStringPlaceholders(
      R"(
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
       base::JoinString(quoted_placement_ids, ", ")},
      nullptr);
  transaction->commands.push_back(std::move(command));

  RunTransaction(std::move(transaction), std::move(callback));
}

void AdEvents::PurgeAllOrphaned(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql = base::ReplaceStringPlaceholders(
      R"(
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
      {GetTableName(), GetTableName()}, nullptr);
  transaction->commands.push_back(std::move(command));

  RunTransaction(std::move(transaction), std::move(callback));
}

std::string AdEvents::GetTableName() const {
  return kTableName;
}

void AdEvents::Create(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
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
          );)";
  transaction->commands.push_back(std::move(command));

  // Optimize database query for `GetUnexpired`.
  CreateTableIndex(transaction, GetTableName(),
                   /*columns=*/{"created_at"});
  CreateTableIndex(transaction, GetTableName(),
                   /*columns=*/{"type", "created_at"});
}

void AdEvents::Migrate(mojom::DBTransactionInfo* transaction,
                       const int to_version) {
  CHECK(transaction);

  switch (to_version) {
    case 5: {
      MigrateToV5(transaction);
      break;
    }

    case 13: {
      MigrateToV13(transaction);
      break;
    }

    case 17: {
      MigrateToV17(transaction);
      break;
    }

    case 28: {
      MigrateToV28(transaction);
      break;
    }

    case 29: {
      MigrateToV29(transaction);
      break;
    }

    case 32: {
      MigrateToV32(transaction);
      break;
    }

    case 35: {
      MigrateToV35(transaction);
      break;
    }

    case 41: {
      MigrateToV41(transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void AdEvents::InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                              const AdEventList& ad_events) {
  CHECK(transaction);

  if (ad_events.empty()) {
    return;
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->sql = BuildInsertOrUpdateSql(&*command, ad_events);
  transaction->commands.push_back(std::move(command));
}

std::string AdEvents::BuildInsertOrUpdateSql(
    mojom::DBCommandInfo* command,
    const AdEventList& ad_events) const {
  CHECK(command);

  const size_t binded_parameters_count = BindParameters(command, ad_events);

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
      {GetTableName(), BuildBindingParameterPlaceholders(
                           /*parameters_count=*/9, binded_parameters_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
