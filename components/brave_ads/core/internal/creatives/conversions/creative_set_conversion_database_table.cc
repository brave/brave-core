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
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_statement_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "creative_set_conversions";

void BindColumnTypes(mojom::DBStatementInfo* const mojom_statement) {
  CHECK(mojom_statement);

  mojom_statement->bind_column_types = {
      mojom::DBBindColumnType::kString,  // creative_set_id
      mojom::DBBindColumnType::kString,  // url_pattern
      mojom::DBBindColumnType::kString,  // verifiable_advertiser_public_key
      mojom::DBBindColumnType::kInt,     // observation_window
      mojom::DBBindColumnType::kTime     // expire_at
  };
}

size_t BindColumns(mojom::DBStatementInfo* mojom_statement,
                   const CreativeSetConversionList& creative_set_conversions) {
  CHECK(mojom_statement);
  CHECK(!creative_set_conversions.empty());

  size_t row_count = 0;

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

    BindColumnString(mojom_statement, index++, creative_set_conversion.id);
    BindColumnString(mojom_statement, index++,
                     creative_set_conversion.url_pattern);
    BindColumnString(mojom_statement, index++,
                     creative_set_conversion
                         .verifiable_advertiser_public_key_base64.value_or(""));
    BindColumnInt(mojom_statement, index++,
                  creative_set_conversion.observation_window.InDays());
    BindColumnTime(mojom_statement, index++,
                   creative_set_conversion.expire_at.value_or(base::Time()));

    ++row_count;
  }

  return row_count;
}

CreativeSetConversionInfo FromMojomRow(
    const mojom::DBRowInfo* const mojom_row) {
  CHECK(mojom_row);

  CreativeSetConversionInfo creative_set_conversion;

  creative_set_conversion.id = ColumnString(mojom_row, 0);
  creative_set_conversion.url_pattern = ColumnString(mojom_row, 1);
  const std::string verifiable_advertiser_public_key_base64 =
      ColumnString(mojom_row, 2);
  if (!verifiable_advertiser_public_key_base64.empty()) {
    creative_set_conversion.verifiable_advertiser_public_key_base64 =
        verifiable_advertiser_public_key_base64;
  }
  creative_set_conversion.observation_window =
      base::Days(ColumnInt(mojom_row, 3));
  const base::Time expire_at = ColumnTime(mojom_row, 4);
  if (!expire_at.is_null()) {
    creative_set_conversion.expire_at = expire_at;
  }

  return creative_set_conversion;
}

void GetCallback(GetCreativeSetConversionsCallback callback,
                 mojom::DBStatementResultInfoPtr mojom_statement_result) {
  if (!mojom_statement_result ||
      mojom_statement_result->result_code !=
          mojom::DBStatementResultInfo::ResultCode::kSuccess) {
    BLOG(0, "Failed to get creative set conversions");

    return std::move(callback).Run(/*success=*/false,
                                   /*conversion_set_conversions=*/{});
  }

  CHECK(mojom_statement_result->rows_union);

  CreativeSetConversionList creative_set_conversions;

  for (const auto& mojom_row : mojom_statement_result->rows_union->get_rows()) {
    const CreativeSetConversionInfo creative_set_conversion =
        FromMojomRow(&*mojom_row);
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

void MigrateToV23(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Recreate table to address a migration problem from older versions.
  DropTable(mojom_transaction, "ad_conversions");

  DropTable(mojom_transaction, "creative_ad_conversions");

  Execute(mojom_transaction, R"(
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
      );)");
}

void MigrateToV28(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Create a temporary table:
  //   - renaming the `expiry_timestamp` column to `expire_at`.
  Execute(mojom_transaction, R"(
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
      );)");

  // Copy legacy columns to the temporary table, drop the legacy table, and
  // rename the temporary table.
  const std::vector<std::string> from_columns = {
      "creative_set_id",    "type",
      "url_pattern",        "advertiser_public_key",
      "observation_window", "expiry_timestamp"};

  const std::vector<std::string> to_columns = {
      "creative_set_id",    "type",     "url_pattern", "advertiser_public_key",
      "observation_window", "expire_at"};

  CopyTableColumns(mojom_transaction, "creative_ad_conversions",
                   "creative_ad_conversions_temp", from_columns, to_columns,
                   /*should_drop=*/true);

  RenameTable(mojom_transaction, "creative_ad_conversions_temp",
              "creative_ad_conversions");
}

void MigrateToV29(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Migrate `expire_at` column from a UNIX timestamp to a WebKit/Chrome
  // timestamp.
  Execute(mojom_transaction, R"(
      UPDATE
        creative_ad_conversions
      SET
        expire_at = (
          CAST(expire_at AS INT64) + 11644473600
        ) * 1000000;)");
}

void MigrateToV30(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Create a temporary table:
  //   - with a new `extract_verifiable_id` column defaulted to `true` for
  //     legacy conversions.
  //   - removing the deprecated `type` column.
  //   - renaming the `advertiser_public_key` column to
  //     `verifiable_advertiser_public_key`.
  Execute(mojom_transaction, R"(
      CREATE TABLE creative_set_conversions_temp (
        creative_set_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        url_pattern TEXT NOT NULL,
        extract_verifiable_id INTEGER NOT NULL DEFAULT 1,
        verifiable_advertiser_public_key TEXT,
        observation_window INTEGER NOT NULL,
        expire_at TIMESTAMP NOT NULL
      );)");

  // Copy legacy columns to the temporary table, drop the legacy table, and
  // rename the temporary table.
  const std::vector<std::string> from_columns = {
      "creative_set_id", "url_pattern", "advertiser_public_key",
      "observation_window", "expire_at"};

  const std::vector<std::string> to_columns = {
      "creative_set_id", "url_pattern", "verifiable_advertiser_public_key",
      "observation_window", "expire_at"};

  CopyTableColumns(mojom_transaction, "creative_ad_conversions",
                   "creative_set_conversions_temp", from_columns, to_columns,
                   /*should_drop=*/true);

  RenameTable(mojom_transaction, "creative_set_conversions_temp",
              "creative_set_conversions");
}

void MigrateToV31(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Create a temporary table:
  //   - removing the deprecated `extract_verifiable_id` column.
  Execute(mojom_transaction, R"(
      CREATE TABLE creative_set_conversions_temp (
        creative_set_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        url_pattern TEXT NOT NULL,
        verifiable_advertiser_public_key TEXT,
        observation_window INTEGER NOT NULL,
        expire_at TIMESTAMP NOT NULL
      );)");

  // Copy legacy columns to the temporary table, drop the legacy table, and
  // rename the temporary table.
  const std::vector<std::string> columns = {"creative_set_id", "url_pattern",
                                            "verifiable_advertiser_public_key",
                                            "observation_window", "expire_at"};

  CopyTableColumns(mojom_transaction, "creative_set_conversions",
                   "creative_set_conversions_temp", columns,
                   /*should_drop=*/true);

  RenameTable(mojom_transaction, "creative_set_conversions_temp",
              "creative_set_conversions");
}

void MigrateToV35(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Optimize database query for `GetUnexpired`.
  CreateTableIndex(mojom_transaction, /*table_name=*/"creative_set_conversions",
                   /*columns=*/{"expire_at"});
}

void MigrateToV43(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Optimize database query for `database::table::AdEvents`.
  CreateTableIndex(mojom_transaction, /*table_name=*/"creative_set_conversions",
                   /*columns=*/{"creative_set_id"});
}

}  // namespace

void CreativeSetConversions::Save(
    const CreativeSetConversionList& creative_set_conversions,
    ResultCallback callback) {
  if (creative_set_conversions.empty()) {
    return std::move(callback).Run(/*success=*/true);
  }

  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();

  Insert(&*mojom_transaction, creative_set_conversions);

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

void CreativeSetConversions::GetUnexpired(
    GetCreativeSetConversionsCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kStep;
  mojom_statement->sql = base::ReplaceStringPlaceholders(
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
      {GetTableName(), TimeToSqlValueAsString(base::Time::Now())}, nullptr);
  BindColumnTypes(&*mojom_statement);
  mojom_transaction->statements.push_back(std::move(mojom_statement));

  GetAdsClient()->RunDBTransaction(
      std::move(mojom_transaction),
      base::BindOnce(&GetCallback, std::move(callback)));
}

void CreativeSetConversions::PurgeExpired(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  Execute(&*mojom_transaction, R"(
            DELETE FROM
              $1
            WHERE
              $2 >= expire_at;)",
          {GetTableName(), TimeToSqlValueAsString(base::Time::Now())});

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

std::string CreativeSetConversions::GetTableName() const {
  return kTableName;
}

void CreativeSetConversions::Create(
    mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  Execute(mojom_transaction, R"(
      CREATE TABLE creative_set_conversions (
        creative_set_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        url_pattern TEXT NOT NULL,
        verifiable_advertiser_public_key TEXT,
        observation_window INTEGER NOT NULL,
        expire_at TIMESTAMP NOT NULL
      );)");

  // Optimize database query for `GetUnexpired` from schema 35.
  CreateTableIndex(mojom_transaction, GetTableName(),
                   /*columns=*/{"expire_at"});

  // Optimize database query for `database::table::AdEvents` from schema 43.
  CreateTableIndex(mojom_transaction, GetTableName(),
                   /*columns=*/{"creative_set_id"});
}

void CreativeSetConversions::Migrate(
    mojom::DBTransactionInfo* mojom_transaction,
    const int to_version) {
  CHECK(mojom_transaction);

  switch (to_version) {
    case 23: {
      MigrateToV23(mojom_transaction);
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

    case 30: {
      MigrateToV30(mojom_transaction);
      break;
    }

    case 31: {
      MigrateToV31(mojom_transaction);
      break;
    }

    case 35: {
      MigrateToV35(mojom_transaction);
      break;
    }

    case 43: {
      MigrateToV43(mojom_transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void CreativeSetConversions::Insert(
    mojom::DBTransactionInfo* mojom_transaction,
    const CreativeSetConversionList& creative_set_conversions) {
  CHECK(mojom_transaction);

  if (creative_set_conversions.empty()) {
    return;
  }

  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type = mojom::DBStatementInfo::OperationType::kRun;
  mojom_statement->sql =
      BuildInsertSql(&*mojom_statement, creative_set_conversions);
  mojom_transaction->statements.push_back(std::move(mojom_statement));
}

std::string CreativeSetConversions::BuildInsertSql(
    mojom::DBStatementInfo* mojom_statement,
    const CreativeSetConversionList& creative_set_conversions) const {
  CHECK(mojom_statement);
  CHECK(!creative_set_conversions.empty());

  const size_t row_count =
      BindColumns(mojom_statement, creative_set_conversions);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            creative_set_id,
            url_pattern,
            verifiable_advertiser_public_key,
            observation_window,
            expire_at
          ) VALUES $2;)",
      {GetTableName(),
       BuildBindColumnPlaceholders(/*column_count=*/5, row_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
