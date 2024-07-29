/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table.h"

#include <cstddef>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_bind_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "creative_set_conversions";

void BindRecords(mojom::DBCommandInfo* const command) {
  CHECK(command);

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // url_pattern
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // verifiable_advertiser_public_key
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,   // observation_window
      mojom::DBCommandInfo::RecordBindingType::INT64_TYPE  // expire_at
  };
}

size_t BindParameters(
    mojom::DBCommandInfo* command,
    const CreativeSetConversionList& creative_set_conversions) {
  CHECK(command);

  size_t count = 0;

  int index = 0;
  for (const auto& creative_set_conversion : creative_set_conversions) {
    if (!creative_set_conversion.IsValid()) {
      // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
      // potential defects using `DumpWithoutCrashing`.
      SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                                "Invalid creative set conversion");
      base::debug::DumpWithoutCrashing();

      BLOG(0, "Invalid creative set conversion");

      continue;
    }

    BindString(command, index++, creative_set_conversion.id);
    BindString(command, index++, creative_set_conversion.url_pattern);
    BindString(command, index++,
               creative_set_conversion.verifiable_advertiser_public_key_base64
                   .value_or(""));
    BindInt(command, index++,
            creative_set_conversion.observation_window.InDays());
    BindInt64(command, index++,
              ToChromeTimestampFromTime(
                  creative_set_conversion.expire_at.value_or(base::Time())));

    ++count;
  }

  return count;
}

CreativeSetConversionInfo GetFromRecord(mojom::DBRecordInfo* const record) {
  CHECK(record);

  CreativeSetConversionInfo creative_set_conversion;

  creative_set_conversion.id = ColumnString(record, 0);
  creative_set_conversion.url_pattern = ColumnString(record, 1);
  const std::string verifiable_advertiser_public_key_base64 =
      ColumnString(record, 2);
  if (!verifiable_advertiser_public_key_base64.empty()) {
    creative_set_conversion.verifiable_advertiser_public_key_base64 =
        verifiable_advertiser_public_key_base64;
  }
  creative_set_conversion.observation_window = base::Days(ColumnInt(record, 3));
  const base::Time expire_at =
      ToTimeFromChromeTimestamp(ColumnInt64(record, 4));
  if (!expire_at.is_null()) {
    creative_set_conversion.expire_at = expire_at;
  }

  return creative_set_conversion;
}

void GetCallback(GetCreativeSetConversionsCallback callback,
                 mojom::DBCommandResponseInfoPtr command_response) {
  if (!command_response ||
      command_response->status !=
          mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get creative set conversions");

    return std::move(callback).Run(/*success=*/false,
                                   /*conversion_set_conversions=*/{});
  }

  CHECK(command_response->result);

  CreativeSetConversionList creative_set_conversions;

  for (const auto& record : command_response->result->get_records()) {
    const CreativeSetConversionInfo creative_set_conversion =
        GetFromRecord(&*record);
    if (!creative_set_conversion.IsValid()) {
      // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
      // potential defects using `DumpWithoutCrashing`.
      SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                                "Invalid creative set conversion");
      base::debug::DumpWithoutCrashing();

      BLOG(0, "Invalid creative set conversion");

      continue;
    }

    creative_set_conversions.push_back(creative_set_conversion);
  }

  std::move(callback).Run(/*success=*/true, creative_set_conversions);
}

void MigrateToV23(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  // Recreate table to address a migration problem from older versions.
  DropTable(transaction, "ad_conversions");

  DropTable(transaction, "creative_ad_conversions");

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
          CREATE TABLE creative_ad_conversions (
            creative_set_id TEXT NOT NULL,
            type TEXT NOT NULL,
            url_pattern TEXT NOT NULL,
            advertiser_public_key TEXT,
            observation_window INTEGER NOT NULL,
            expiry_timestamp TIMESTAMP NOT NULL,
            PRIMARY KEY (
              creative_set_id,
              type
            ) ON CONFLICT REPLACE
          );)";
  transaction->commands.push_back(std::move(command));
}

void MigrateToV28(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  // Create a temporary table:
  //   - renaming the `expiry_timestamp` column to `expire_at`.
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
          CREATE TABLE creative_ad_conversions_temp (
            creative_set_id TEXT NOT NULL,
            type TEXT NOT NULL,
            url_pattern TEXT NOT NULL,
            advertiser_public_key TEXT,
            observation_window INTEGER NOT NULL,
            expire_at TIMESTAMP NOT NULL,
            PRIMARY KEY (
              creative_set_id,
              type
            ) ON CONFLICT REPLACE
          );)";
  transaction->commands.push_back(std::move(command));

  // Copy legacy columns to the temporary table, drop the legacy table, and
  // rename the temporary table.
  const std::vector<std::string> from_columns = {
      "creative_set_id",    "type",
      "url_pattern",        "advertiser_public_key",
      "observation_window", "expiry_timestamp"};

  const std::vector<std::string> to_columns = {
      "creative_set_id",    "type",     "url_pattern", "advertiser_public_key",
      "observation_window", "expire_at"};

  CopyTableColumns(transaction, "creative_ad_conversions",
                   "creative_ad_conversions_temp", from_columns, to_columns,
                   /*should_drop=*/true);

  RenameTable(transaction, "creative_ad_conversions_temp",
              "creative_ad_conversions");
}

void MigrateToV29(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  // Migrate `expire_at` column from a UNIX timestamp to a WebKit/Chrome
  // timestamp.
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
          UPDATE
            creative_ad_conversions
          SET
            expire_at = (
              CAST(expire_at AS INT64) + 11644473600
            ) * 1000000;)";
  transaction->commands.push_back(std::move(command));
}

void MigrateToV30(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  // Create a temporary table:
  //   - with a new `extract_verifiable_id` column defaulted to `true` for
  //     legacy conversions.
  //   - removing the deprecated `type` column.
  //   - renaming the `advertiser_public_key` column to
  //     `verifiable_advertiser_public_key`.
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
          CREATE TABLE creative_set_conversions_temp (
            creative_set_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
            url_pattern TEXT NOT NULL,
            extract_verifiable_id INTEGER NOT NULL DEFAULT 1,
            verifiable_advertiser_public_key TEXT,
            observation_window INTEGER NOT NULL,
            expire_at TIMESTAMP NOT NULL
          );)";
  transaction->commands.push_back(std::move(command));

  // Copy legacy columns to the temporary table, drop the legacy table, and
  // rename the temporary table.
  const std::vector<std::string> from_columns = {
      "creative_set_id", "url_pattern", "advertiser_public_key",
      "observation_window", "expire_at"};

  const std::vector<std::string> to_columns = {
      "creative_set_id", "url_pattern", "verifiable_advertiser_public_key",
      "observation_window", "expire_at"};

  CopyTableColumns(transaction, "creative_ad_conversions",
                   "creative_set_conversions_temp", from_columns, to_columns,
                   /*should_drop=*/true);

  RenameTable(transaction, "creative_set_conversions_temp",
              "creative_set_conversions");
}

void MigrateToV31(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  // Create a temporary table:
  //   - removing the deprecated `extract_verifiable_id` column.
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
          CREATE TABLE creative_set_conversions_temp (
            creative_set_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
            url_pattern TEXT NOT NULL,
            verifiable_advertiser_public_key TEXT,
            observation_window INTEGER NOT NULL,
            expire_at TIMESTAMP NOT NULL
          );)";
  transaction->commands.push_back(std::move(command));

  // Copy legacy columns to the temporary table, drop the legacy table, and
  // rename the temporary table.
  const std::vector<std::string> columns = {"creative_set_id", "url_pattern",
                                            "verifiable_advertiser_public_key",
                                            "observation_window", "expire_at"};

  CopyTableColumns(transaction, "creative_set_conversions",
                   "creative_set_conversions_temp", columns,
                   /*should_drop=*/true);

  RenameTable(transaction, "creative_set_conversions_temp",
              "creative_set_conversions");
}

void MigrateToV35(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  // Optimize database query for `GetUnexpired`.
  CreateTableIndex(transaction, /*table_name=*/"creative_set_conversions",
                   /*columns=*/{"expire_at"});
}

}  // namespace

void CreativeSetConversions::Save(
    const CreativeSetConversionList& creative_set_conversions,
    ResultCallback callback) {
  if (creative_set_conversions.empty()) {
    return std::move(callback).Run(/*success=*/true);
  }

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  InsertOrUpdate(&*transaction, creative_set_conversions);

  RunTransaction(std::move(transaction), std::move(callback));
}

void CreativeSetConversions::GetUnexpired(
    GetCreativeSetConversionsCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            creative_set_id,
            url_pattern,
            verifiable_advertiser_public_key,
            observation_window,
            expire_at
          FROM
            $1
          WHERE
            $2 < expire_at;)",
      {GetTableName(),
       base::NumberToString(ToChromeTimestampFromTime(base::Time::Now()))},
      nullptr);
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  RunDBTransaction(std::move(transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void CreativeSetConversions::PurgeExpired(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql = base::ReplaceStringPlaceholders(
      R"(
          DELETE FROM
            $1
          WHERE
            $2 >= expire_at;)",
      {GetTableName(),
       base::NumberToString(ToChromeTimestampFromTime(base::Time::Now()))},
      nullptr);
  transaction->commands.push_back(std::move(command));

  RunTransaction(std::move(transaction), std::move(callback));
}

std::string CreativeSetConversions::GetTableName() const {
  return kTableName;
}

void CreativeSetConversions::Create(
    mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
          CREATE TABLE creative_set_conversions (
            creative_set_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
            url_pattern TEXT NOT NULL,
            verifiable_advertiser_public_key TEXT,
            observation_window INTEGER NOT NULL,
            expire_at TIMESTAMP NOT NULL
          );)";
  transaction->commands.push_back(std::move(command));

  // Optimize database query for `GetUnexpired`.
  CreateTableIndex(transaction, GetTableName(), /*columns=*/{"expire_at"});
}

void CreativeSetConversions::Migrate(mojom::DBTransactionInfo* transaction,
                                     const int to_version) {
  CHECK(transaction);

  switch (to_version) {
    case 23: {
      MigrateToV23(transaction);
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

    case 30: {
      MigrateToV30(transaction);
      break;
    }

    case 31: {
      MigrateToV31(transaction);
      break;
    }

    case 35: {
      MigrateToV35(transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void CreativeSetConversions::InsertOrUpdate(
    mojom::DBTransactionInfo* transaction,
    const CreativeSetConversionList& creative_set_conversions) {
  CHECK(transaction);

  if (creative_set_conversions.empty()) {
    return;
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->sql = BuildInsertOrUpdateSql(&*command, creative_set_conversions);
  transaction->commands.push_back(std::move(command));
}

std::string CreativeSetConversions::BuildInsertOrUpdateSql(
    mojom::DBCommandInfo* command,
    const CreativeSetConversionList& creative_set_conversions) const {
  CHECK(command);

  const size_t binded_parameters_count =
      BindParameters(command, creative_set_conversions);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            creative_set_id,
            url_pattern,
            verifiable_advertiser_public_key,
            observation_window,
            expire_at
          ) VALUES $2;)",
      {GetTableName(), BuildBindingParameterPlaceholders(
                           /*parameters_count=*/5, binded_parameters_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
