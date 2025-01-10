/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"

#include <cstddef>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_statement_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "ad_events";

void BindColumnTypes(const mojom::DBActionInfoPtr& mojom_db_action) {
  CHECK(mojom_db_action);

  mojom_db_action->bind_column_types = {
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

size_t BindColumns(const mojom::DBActionInfoPtr& mojom_db_action,
                   const AdEventList& ad_events) {
  CHECK(mojom_db_action);
  CHECK(!ad_events.empty());

  size_t row_count = 0;

  int index = 0;
  for (const auto& ad_event : ad_events) {
    if (!ad_event.IsValid()) {
      BLOG(0, "Invalid ad event");

      continue;
    }

    BindColumnString(mojom_db_action, index++, ad_event.placement_id);
    BindColumnString(mojom_db_action, index++, ToString(ad_event.type));
    BindColumnString(mojom_db_action, index++,
                     ToString(ad_event.confirmation_type));
    BindColumnString(mojom_db_action, index++, ad_event.campaign_id);
    BindColumnString(mojom_db_action, index++, ad_event.creative_set_id);
    BindColumnString(mojom_db_action, index++, ad_event.creative_instance_id);
    BindColumnString(mojom_db_action, index++, ad_event.advertiser_id);
    BindColumnString(mojom_db_action, index++, ad_event.segment);
    BindColumnTime(mojom_db_action, index++,
                   ad_event.created_at.value_or(base::Time()));

    ++row_count;
  }

  return row_count;
}

AdEventInfo FromMojomRow(const mojom::DBRowInfoPtr& mojom_db_row) {
  CHECK(mojom_db_row);

  AdEventInfo ad_event;

  ad_event.placement_id = ColumnString(mojom_db_row, 0);
  ad_event.type = ToMojomAdType(ColumnString(mojom_db_row, 1));
  ad_event.confirmation_type =
      ToMojomConfirmationType(ColumnString(mojom_db_row, 2));
  ad_event.campaign_id = ColumnString(mojom_db_row, 3);
  ad_event.creative_set_id = ColumnString(mojom_db_row, 4);
  ad_event.creative_instance_id = ColumnString(mojom_db_row, 5);
  ad_event.advertiser_id = ColumnString(mojom_db_row, 6);
  ad_event.segment = ColumnString(mojom_db_row, 7);
  const base::Time created_at = ColumnTime(mojom_db_row, 8);
  if (!created_at.is_null()) {
    ad_event.created_at = created_at;
  }

  return ad_event;
}

void GetCallback(
    GetAdEventsCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (IsError(mojom_db_transaction_result)) {
    BLOG(0, "Failed to get ad events");
    return std::move(callback).Run(/*success=*/false, /*ad_events=*/{});
  }

  CHECK(mojom_db_transaction_result->rows_union);

  AdEventList ad_events;
  for (const auto& mojom_db_row :
       mojom_db_transaction_result->rows_union->get_rows()) {
    const AdEventInfo ad_event = FromMojomRow(mojom_db_row);
    if (!ad_event.IsValid()) {
      BLOG(0, "Invalid ad event");

      continue;
    }

    ad_events.push_back(ad_event);
  }

  std::move(callback).Run(/*success=*/true, ad_events);
}

void MigrateToV35(const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  DropTableIndex(mojom_db_transaction, "ad_events_created_at_index");

  // Optimize database query for `GetUnexpired`.
  CreateTableIndex(mojom_db_transaction, /*table_name=*/"ad_events",
                   /*columns=*/{"created_at"});
  CreateTableIndex(mojom_db_transaction, /*table_name=*/"ad_events",
                   /*columns=*/{"type", "created_at"});
}

void MigrateToV41(const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // Remove non-clicked search result ad events for users who have not joined
  // Brave Rewards.
  if (!UserHasJoinedBraveRewards()) {
    Execute(mojom_db_transaction, R"(
        DELETE FROM
          ad_events
        WHERE
          type == 'search_result_ad'
          AND confirmation_type != 'click';)");
  }
}

void MigrateToV43(const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  DropTableIndex(mojom_db_transaction, "ad_events_type_creative_set_id_index");

  CreateTableIndex(mojom_db_transaction, /*table_name=*/"ad_events",
                   /*columns=*/{"type"});
  CreateTableIndex(mojom_db_transaction, /*table_name=*/"ad_events",
                   /*columns=*/{"confirmation_type"});
  CreateTableIndex(mojom_db_transaction, /*table_name=*/"ad_events",
                   /*columns=*/{"creative_set_id"});
  CreateTableIndex(mojom_db_transaction, /*table_name=*/"ad_events",
                   /*columns=*/{"placement_id"});
}

}  // namespace

void AdEvents::RecordEvent(const AdEventInfo& ad_event,
                           ResultCallback callback) {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  Insert(mojom_db_transaction, {ad_event});

  RunDBTransaction(FROM_HERE, std::move(mojom_db_transaction),
                   std::move(callback));
}

void AdEvents::GetAll(GetAdEventsCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kStepStatement;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
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
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunDBTransaction(FROM_HERE, std::move(mojom_db_transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void AdEvents::Get(mojom::AdType mojom_ad_type,
                   mojom::ConfirmationType mojom_confirmation_type,
                   base::TimeDelta time_window,
                   GetAdEventsCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kStepStatement;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
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
            AND confirmation_type = '$3'
            AND created_at > $4
          ORDER BY
            created_at ASC;)",
      {GetTableName(), ToString(mojom_ad_type),
       ToString(mojom_confirmation_type),
       TimeToSqlValueAsString(base::Time::Now() - time_window)},
      nullptr);
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunDBTransaction(FROM_HERE, std::move(mojom_db_transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void AdEvents::GetUnexpired(GetAdEventsCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kStepStatement;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
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
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunDBTransaction(FROM_HERE, std::move(mojom_db_transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void AdEvents::GetUnexpired(mojom::AdType mojom_ad_type,
                            GetAdEventsCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kStepStatement;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
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
      {GetTableName(), ToString(mojom_ad_type),
       TimeToSqlValueAsString(base::Time::Now() - base::Days(90))},
      nullptr);
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunDBTransaction(FROM_HERE, std::move(mojom_db_transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void AdEvents::PurgeExpired(ResultCallback callback) const {
  const size_t days = UserHasJoinedBraveRewards() ? 90 : 30;

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  Execute(mojom_db_transaction, R"(
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

  RunDBTransaction(FROM_HERE, std::move(mojom_db_transaction),
                   std::move(callback));
}

void AdEvents::PurgeOrphaned(mojom::AdType mojom_ad_type,
                             ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  Execute(mojom_db_transaction, R"(
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
          {GetTableName(), GetTableName(), ToString(mojom_ad_type)});

  RunDBTransaction(FROM_HERE, std::move(mojom_db_transaction),
                   std::move(callback));
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

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  Execute(mojom_db_transaction, R"(
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

  RunDBTransaction(FROM_HERE, std::move(mojom_db_transaction),
                   std::move(callback));
}

void AdEvents::PurgeAllOrphaned(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  Execute(mojom_db_transaction, R"(
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

  RunDBTransaction(FROM_HERE, std::move(mojom_db_transaction),
                   std::move(callback));
}

std::string AdEvents::GetTableName() const {
  return kTableName;
}

void AdEvents::Create(const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
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

  // Optimize database query for `GetUnexpired`, and `PurgeExpired` from
  // schema 35.
  CreateTableIndex(mojom_db_transaction, GetTableName(),
                   /*columns=*/{"created_at"});

  // Optimize database query for `GetUnexpired`, and `PurgeExpired` from
  // schema 43.
  CreateTableIndex(mojom_db_transaction, GetTableName(),
                   /*columns=*/{"creative_set_id"});

  // Optimize database query for `GetUnexpired`, and `PurgeOrphaned` from
  // schema 43.
  CreateTableIndex(mojom_db_transaction, GetTableName(),
                   /*columns=*/{"type"});

  // Optimize database query for `PurgeOrphaned`, and `PurgeAllOrphaned` from
  // schema 43.
  CreateTableIndex(mojom_db_transaction, GetTableName(),
                   /*columns=*/{"confirmation_type"});
  CreateTableIndex(mojom_db_transaction, GetTableName(),
                   /*columns=*/{"placement_id"});
}

void AdEvents::Migrate(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
                       int to_version) {
  CHECK(mojom_db_transaction);

  switch (to_version) {
    case 35: {
      MigrateToV35(mojom_db_transaction);
      break;
    }

    case 41: {
      MigrateToV41(mojom_db_transaction);
      break;
    }

    case 43: {
      MigrateToV43(mojom_db_transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void AdEvents::Insert(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
                      const AdEventList& ad_events) {
  CHECK(mojom_db_transaction);

  if (ad_events.empty()) {
    return;
  }

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kRunStatement;
  mojom_db_action->sql = BuildInsertSql(mojom_db_action, ad_events);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

std::string AdEvents::BuildInsertSql(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const AdEventList& ad_events) const {
  CHECK(mojom_db_action);
  CHECK(!ad_events.empty());

  const size_t row_count = BindColumns(mojom_db_action, ad_events);

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
