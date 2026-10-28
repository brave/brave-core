/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_database_table.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_statement_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_database_table_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "campaign_conversions";

void BindColumnTypes(const mojom::DBActionInfoPtr& mojom_db_action) {
  CHECK(mojom_db_action);

  mojom_db_action->bind_column_types = {
      mojom::DBBindColumnType::kString,  // campaign_id
      mojom::DBBindColumnType::kString,  // url_pattern
      mojom::DBBindColumnType::kInt,     // observation_window
      mojom::DBBindColumnType::kTime     // expire_at
  };
}

size_t BindColumns(const mojom::DBActionInfoPtr& mojom_db_action,
                   const CampaignConversionList& campaign_conversions) {
  CHECK(mojom_db_action);
  CHECK(!campaign_conversions.empty());

  size_t row_count = 0;

  int32_t index = 0;
  for (const auto& campaign_conversion : campaign_conversions) {
    CHECK(campaign_conversion.IsValid());

    BindColumnString(mojom_db_action, index++, campaign_conversion.id);
    BindColumnString(mojom_db_action, index++, campaign_conversion.url_pattern);
    BindColumnInt(mojom_db_action, index++,
                  campaign_conversion.observation_window.InDays());
    BindColumnTime(mojom_db_action, index++,
                   campaign_conversion.expire_at.value_or(base::Time()));

    ++row_count;
  }

  return row_count;
}

void GetCallback(
    GetCampaignConversionsCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (!IsTransactionSuccessful(mojom_db_transaction_result)) {
    BLOG(0, "Failed to get campaign conversions");
    return std::move(callback).Run(/*success=*/false,
                                   /*conversion_set_conversions=*/{});
  }

  CHECK(mojom_db_transaction_result->rows_union);

  CampaignConversionList campaign_conversions;

  for (const auto& mojom_db_row :
       mojom_db_transaction_result->rows_union->get_rows()) {
    const CampaignConversionInfo campaign_conversion =
        CampaignConversionFromMojomRow(mojom_db_row);
    if (!campaign_conversion.IsValid()) {
      BLOG(0, "Invalid campaign conversion");
      continue;
    }

    campaign_conversions.push_back(campaign_conversion);
  }

  std::move(callback).Run(/*success=*/true, campaign_conversions);
}

std::string BuildInsertSql(const mojom::DBActionInfoPtr& mojom_db_action,
                           const CampaignConversionList& campaign_conversions) {
  CHECK(mojom_db_action);
  CHECK(!campaign_conversions.empty());

  const size_t row_count = BindColumns(mojom_db_action, campaign_conversions);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            campaign_id,
            url_pattern,
            observation_window,
            expire_at
          ) VALUES $2)",
      {kTableName, BuildBindColumnPlaceholders(/*column_count=*/4, row_count)},
      nullptr);
}

void Insert(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
            const CampaignConversionList& campaign_conversions) {
  CHECK(mojom_db_transaction);
  CHECK(!campaign_conversions.empty());

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteWithBindings;
  mojom_db_action->sql = BuildInsertSql(mojom_db_action, campaign_conversions);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

void MigrateToV55(const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // Drop the `verifiable_advertiser_public_key` column which is no longer used
  // now that verifiable conversions have been removed.
  //
  // The table was still named `creative_set_conversions` at this schema
  // version, so the literal name below is intentionally not
  // `campaign_conversions`.
  RenameTable(mojom_db_transaction,
              /*from=*/"creative_set_conversions",
              /*to=*/"creative_set_conversions_temp");

  Execute(mojom_db_transaction, R"(
      CREATE TABLE creative_set_conversions (
        creative_set_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        url_pattern TEXT NOT NULL,
        observation_window INTEGER NOT NULL,
        expire_at TIMESTAMP NOT NULL
      ))");

  const std::vector<std::string> columns = {"creative_set_id", "url_pattern",
                                            "observation_window", "expire_at"};

  CopyTableColumns(mojom_db_transaction,
                   /*from=*/"creative_set_conversions_temp",
                   /*to=*/"creative_set_conversions", columns,
                   /*should_drop=*/true);

  CreateTableIndex(mojom_db_transaction,
                   /*table_name=*/"creative_set_conversions",
                   /*columns=*/{"expire_at"});

  CreateTableIndex(mojom_db_transaction,
                   /*table_name=*/"creative_set_conversions",
                   /*columns=*/{"creative_set_id"});
}

void MigrateToV61(const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // Renamed from `creative_set_conversions` and keyed by `campaign_id` rather
  // than `creative_set_id`. Conversions are short lived and expire quickly,
  // so existing rows are dropped rather than migrated.
  DropTable(mojom_db_transaction, "creative_set_conversions");

  Execute(mojom_db_transaction, R"(
      CREATE TABLE campaign_conversions (
        campaign_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        url_pattern TEXT NOT NULL,
        observation_window INTEGER NOT NULL,
        expire_at TIMESTAMP NOT NULL
      ))");

  CreateTableIndex(mojom_db_transaction,
                   /*table_name=*/"campaign_conversions",
                   /*columns=*/{"expire_at"});

  // No index needed on `campaign_id`; it is the primary key, so SQLite
  // already maintains an index for it.
}

}  // namespace

void CampaignConversions::Save(
    const CampaignConversionList& campaign_conversions,
    ResultCallback callback) {
  CampaignConversionList filtered_campaign_conversions;
  std::copy_if(campaign_conversions.cbegin(), campaign_conversions.cend(),
               std::back_inserter(filtered_campaign_conversions),
               [](const CampaignConversionInfo& campaign_conversion) {
                 if (!campaign_conversion.IsValid()) {
                   BLOG(0, "Invalid campaign conversion");
                   return false;
                 }
                 return true;
               });
  if (filtered_campaign_conversions.empty()) {
    return std::move(callback).Run(/*success=*/true);
  }

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  Insert(mojom_db_transaction, filtered_campaign_conversions);

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 std::move(callback));
}

void CampaignConversions::GetUnexpired(
    GetCampaignConversionsCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteQueryWithBindings;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            campaign_id,
            url_pattern,
            observation_window,
            expire_at
          FROM
            $1
          WHERE
            $2 < expire_at)",
      {kTableName, TimeToSqlValueAsString(base::Time::Now())}, nullptr);
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 base::BindOnce(&GetCallback, std::move(callback)));
}

void CampaignConversions::GetActive(
    GetCampaignConversionsCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteQueryWithBindings;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT DISTINCT
            campaign_conversion.campaign_id,
            campaign_conversion.url_pattern,
            campaign_conversion.observation_window,
            campaign_conversion.expire_at
          FROM
            $1 AS campaign_conversion
            INNER JOIN ad_events
              ON ad_events.campaign_id = campaign_conversion.campaign_id
          WHERE
            $2 < expire_at
            AND ad_events.confirmation_type IN ('$3', '$4'))",
      {kTableName, TimeToSqlValueAsString(base::Time::Now()),
       std::string(ToString(mojom::ConfirmationType::kViewedImpression)),
       std::string(ToString(mojom::ConfirmationType::kClicked))},
      nullptr);
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 base::BindOnce(&GetCallback, std::move(callback)));
}

void CampaignConversions::PurgeExpired(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  Execute(mojom_db_transaction, R"(
            DELETE FROM
              $1
            WHERE
              $2 >= expire_at)",
          {kTableName, TimeToSqlValueAsString(base::Time::Now())});

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 std::move(callback));
}

void CampaignConversions::Create(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE campaign_conversions (
        campaign_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        url_pattern TEXT NOT NULL,
        observation_window INTEGER NOT NULL,
        expire_at TIMESTAMP NOT NULL
      ))");

  // Optimize database query for `GetUnexpired` from schema 35.
  CreateTableIndex(mojom_db_transaction, kTableName,
                   /*columns=*/{"expire_at"});

  // No index needed on `campaign_id`; it is the primary key, so SQLite
  // already maintains an index for it.
}

void CampaignConversions::Migrate(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    int to_version) {
  CHECK(mojom_db_transaction);

  switch (to_version) {
    case 55: {
      MigrateToV55(mojom_db_transaction);
      break;
    }

    case 61: {
      MigrateToV61(mojom_db_transaction);
      break;
    }

    default: {
      // No migration needed.
      break;
    }
  }
}

}  // namespace brave_ads::database::table
