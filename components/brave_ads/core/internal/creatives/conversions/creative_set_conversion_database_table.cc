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
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "creative_set_conversions";

void BindColumnTypes(const mojom::DBActionInfoPtr& mojom_db_action) {
  CHECK(mojom_db_action);

  mojom_db_action->bind_column_types = {
      mojom::DBBindColumnType::kString,  // creative_set_id
      mojom::DBBindColumnType::kString,  // url_pattern
      mojom::DBBindColumnType::kString,  // verifiable_advertiser_public_key
      mojom::DBBindColumnType::kInt,     // observation_window
      mojom::DBBindColumnType::kTime     // expire_at
  };
}

size_t BindColumns(const mojom::DBActionInfoPtr& mojom_db_action,
                   const CreativeSetConversionList& creative_set_conversions) {
  CHECK(mojom_db_action);
  CHECK(!creative_set_conversions.empty());

  size_t row_count = 0;

  int index = 0;
  for (const auto& creative_set_conversion : creative_set_conversions) {
    if (!creative_set_conversion.IsValid()) {
      // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
      // potential defects using `DumpWithoutCrashing`.
      SCOPED_CRASH_KEY_STRING64("Issue32066", "creative_set_conversion_id",
                                creative_set_conversion.id);
      SCOPED_CRASH_KEY_STRING256("Issue32066", "url_pattern",
                                 creative_set_conversion.url_pattern);
      SCOPED_CRASH_KEY_NUMBER(
          "Issue32066", "observation_window",
          creative_set_conversion.observation_window.InDays());
      SCOPED_CRASH_KEY_NUMBER(
          "Issue32066", "expire_at",
          creative_set_conversion.expire_at->ToDeltaSinceWindowsEpoch()
              .InMicroseconds());
      SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                                "Invalid creative set conversion");
      base::debug::DumpWithoutCrashing();

      BLOG(0, "Invalid creative set conversion");

      continue;
    }

    BindColumnString(mojom_db_action, index++, creative_set_conversion.id);
    BindColumnString(mojom_db_action, index++,
                     creative_set_conversion.url_pattern);
    BindColumnString(mojom_db_action, index++,
                     creative_set_conversion
                         .verifiable_advertiser_public_key_base64.value_or(""));
    BindColumnInt(mojom_db_action, index++,
                  creative_set_conversion.observation_window.InDays());
    BindColumnTime(mojom_db_action, index++,
                   creative_set_conversion.expire_at.value_or(base::Time()));

    ++row_count;
  }

  return row_count;
}

CreativeSetConversionInfo FromMojomRow(
    const mojom::DBRowInfoPtr& mojom_db_row) {
  CHECK(mojom_db_row);

  CreativeSetConversionInfo creative_set_conversion;

  creative_set_conversion.id = ColumnString(mojom_db_row, 0);
  creative_set_conversion.url_pattern = ColumnString(mojom_db_row, 1);
  const std::string verifiable_advertiser_public_key_base64 =
      ColumnString(mojom_db_row, 2);
  if (!verifiable_advertiser_public_key_base64.empty()) {
    creative_set_conversion.verifiable_advertiser_public_key_base64 =
        verifiable_advertiser_public_key_base64;
  }
  creative_set_conversion.observation_window =
      base::Days(ColumnInt(mojom_db_row, 3));
  const base::Time expire_at = ColumnTime(mojom_db_row, 4);
  if (!expire_at.is_null()) {
    creative_set_conversion.expire_at = expire_at;
  }

  return creative_set_conversion;
}

void GetCallback(
    GetCreativeSetConversionsCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (IsError(mojom_db_transaction_result)) {
    BLOG(0, "Failed to get creative set conversions");
    return std::move(callback).Run(/*success=*/false,
                                   /*conversion_set_conversions=*/{});
  }

  CHECK(mojom_db_transaction_result->rows_union);

  CreativeSetConversionList creative_set_conversions;

  for (const auto& mojom_db_row :
       mojom_db_transaction_result->rows_union->get_rows()) {
    const CreativeSetConversionInfo creative_set_conversion =
        FromMojomRow(mojom_db_row);
    if (!creative_set_conversion.IsValid()) {
      // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
      // potential defects using `DumpWithoutCrashing`.
      SCOPED_CRASH_KEY_STRING64("Issue32066", "creative_set_conversion_id",
                                creative_set_conversion.id);
      SCOPED_CRASH_KEY_STRING256("Issue32066", "url_pattern",
                                 creative_set_conversion.url_pattern);
      SCOPED_CRASH_KEY_NUMBER(
          "Issue32066", "observation_window",
          creative_set_conversion.observation_window.InDays());
      SCOPED_CRASH_KEY_NUMBER(
          "Issue32066", "expire_at",
          creative_set_conversion.expire_at->ToDeltaSinceWindowsEpoch()
              .InMicroseconds());
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

void MigrateToV35(const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // Optimize database query for `GetUnexpired`.
  CreateTableIndex(mojom_db_transaction,
                   /*table_name=*/"creative_set_conversions",
                   /*columns=*/{"expire_at"});
}

void MigrateToV43(const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // Optimize database query for `database::table::AdEvents`.
  CreateTableIndex(mojom_db_transaction,
                   /*table_name=*/"creative_set_conversions",
                   /*columns=*/{"creative_set_id"});
}

}  // namespace

void CreativeSetConversions::Save(
    const CreativeSetConversionList& creative_set_conversions,
    ResultCallback callback) {
  if (creative_set_conversions.empty()) {
    return std::move(callback).Run(/*success=*/true);
  }

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  Insert(mojom_db_transaction, creative_set_conversions);

  RunDBTransaction(std::move(mojom_db_transaction), std::move(callback));
}

void CreativeSetConversions::GetUnexpired(
    GetCreativeSetConversionsCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kStepStatement;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
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
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunDBTransaction(std::move(mojom_db_transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void CreativeSetConversions::GetActive(
    GetCreativeSetConversionsCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kStepStatement;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            creative_set_conversion.creative_set_id,
            creative_set_conversion.url_pattern,
            creative_set_conversion.verifiable_advertiser_public_key,
            creative_set_conversion.observation_window,
            creative_set_conversion.expire_at
          FROM
            $1 AS creative_set_conversion
            INNER JOIN ad_events ON ad_events.creative_set_id = creative_set_conversion.creative_set_id
          WHERE
            $2 < expire_at
            AND ad_events.confirmation_type IN ('$3', '$4');)",
      {GetTableName(), TimeToSqlValueAsString(base::Time::Now()),
       ToString(mojom::ConfirmationType::kViewedImpression),
       ToString(mojom::ConfirmationType::kClicked)},
      nullptr);
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunDBTransaction(std::move(mojom_db_transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void CreativeSetConversions::PurgeExpired(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  Execute(mojom_db_transaction, R"(
            DELETE FROM
              $1
            WHERE
              $2 >= expire_at;)",
          {GetTableName(), TimeToSqlValueAsString(base::Time::Now())});

  RunDBTransaction(std::move(mojom_db_transaction), std::move(callback));
}

std::string CreativeSetConversions::GetTableName() const {
  return kTableName;
}

void CreativeSetConversions::Create(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE creative_set_conversions (
        creative_set_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        url_pattern TEXT NOT NULL,
        verifiable_advertiser_public_key TEXT,
        observation_window INTEGER NOT NULL,
        expire_at TIMESTAMP NOT NULL
      );)");

  // Optimize database query for `GetUnexpired` from schema 35.
  CreateTableIndex(mojom_db_transaction, GetTableName(),
                   /*columns=*/{"expire_at"});

  // Optimize database query for `database::table::AdEvents` from schema 43.
  CreateTableIndex(mojom_db_transaction, GetTableName(),
                   /*columns=*/{"creative_set_id"});
}

void CreativeSetConversions::Migrate(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    int to_version) {
  CHECK(mojom_db_transaction);

  switch (to_version) {
    case 35: {
      MigrateToV35(mojom_db_transaction);
      break;
    }

    case 43: {
      MigrateToV43(mojom_db_transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void CreativeSetConversions::Insert(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    const CreativeSetConversionList& creative_set_conversions) {
  CHECK(mojom_db_transaction);

  if (creative_set_conversions.empty()) {
    return;
  }

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kRunStatement;
  mojom_db_action->sql =
      BuildInsertSql(mojom_db_action, creative_set_conversions);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

std::string CreativeSetConversions::BuildInsertSql(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const CreativeSetConversionList& creative_set_conversions) const {
  CHECK(mojom_db_action);
  CHECK(!creative_set_conversions.empty());

  const size_t row_count =
      BindColumns(mojom_db_action, creative_set_conversions);

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
