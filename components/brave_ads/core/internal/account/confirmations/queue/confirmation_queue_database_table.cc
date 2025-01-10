/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_database_table.h"

#include <cstddef>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/location.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_feature.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/containers/container_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_statement_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "confirmation_queue";

constexpr int kDefaultBatchSize = 50;

constexpr base::TimeDelta kMaximumRetryDelay = base::Hours(1);

void BindColumnTypes(const mojom::DBActionInfoPtr& mojom_db_action) {
  CHECK(mojom_db_action);

  mojom_db_action->bind_column_types = {
      mojom::DBBindColumnType::kString,  // transaction_id
      mojom::DBBindColumnType::kString,  // creative_instance_id
      mojom::DBBindColumnType::kString,  // type
      mojom::DBBindColumnType::kString,  // ad_type
      mojom::DBBindColumnType::kTime,    // created_at
      mojom::DBBindColumnType::kString,  // token
      mojom::DBBindColumnType::kString,  // blinded_token
      mojom::DBBindColumnType::kString,  // unblinded_token
      mojom::DBBindColumnType::kString,  // public_key
      mojom::DBBindColumnType::kString,  // signature
      mojom::DBBindColumnType::kString,  // credential_base64url
      mojom::DBBindColumnType::kString,  // user_data
      mojom::DBBindColumnType::kTime,    // process_at
      mojom::DBBindColumnType::kInt      // retry_count
  };
}

size_t BindColumns(const mojom::DBActionInfoPtr& mojom_db_action,
                   const ConfirmationQueueItemList& confirmation_queue_items) {
  CHECK(mojom_db_action);
  CHECK(!confirmation_queue_items.empty());

  size_t row_count = 0;

  int index = 0;
  for (const auto& confirmation_queue_item : confirmation_queue_items) {
    if (!confirmation_queue_item.IsValid()) {
      BLOG(0, "Invalid confirmation queue item");

      continue;
    }

    // The queue does not store dynamic user data for a confirmation due to the
    // token redemption process which rebuilds the confirmation. Hence, we must
    // regenerate the confirmation without the dynamic user data.
    const ConfirmationInfo confirmation =
        RebuildConfirmationWithoutDynamicUserData(
            confirmation_queue_item.confirmation);

    BindColumnString(mojom_db_action, index++, confirmation.transaction_id);

    BindColumnString(mojom_db_action, index++,
                     confirmation.creative_instance_id);

    BindColumnString(mojom_db_action, index++, ToString(confirmation.type));

    BindColumnString(mojom_db_action, index++, ToString(confirmation.ad_type));

    BindColumnTime(mojom_db_action, index++,
                   confirmation.created_at.value_or(base::Time()));

    if (confirmation.reward) {
      BindColumnString(mojom_db_action, index++,
                       confirmation.reward->token.EncodeBase64().value_or(""));

      BindColumnString(
          mojom_db_action, index++,
          confirmation.reward->blinded_token.EncodeBase64().value_or(""));

      BindColumnString(
          mojom_db_action, index++,
          confirmation.reward->unblinded_token.EncodeBase64().value_or(""));

      BindColumnString(
          mojom_db_action, index++,
          confirmation.reward->public_key.EncodeBase64().value_or(""));

      BindColumnString(mojom_db_action, index++,
                       confirmation.reward->signature);

      BindColumnString(mojom_db_action, index++,
                       confirmation.reward->credential_base64url);
    } else {
      BindColumnString(mojom_db_action, index++, "");
      BindColumnString(mojom_db_action, index++, "");
      BindColumnString(mojom_db_action, index++, "");
      BindColumnString(mojom_db_action, index++, "");
      BindColumnString(mojom_db_action, index++, "");
      BindColumnString(mojom_db_action, index++, "");
    }

    std::string user_data_json;
    CHECK(
        base::JSONWriter::Write(confirmation.user_data.fixed, &user_data_json));
    BindColumnString(mojom_db_action, index++, user_data_json);

    BindColumnTime(mojom_db_action, index++,
                   confirmation_queue_item.process_at.value_or(base::Time()));

    BindColumnInt(mojom_db_action, index++,
                  confirmation_queue_item.retry_count);

    ++row_count;
  }

  return row_count;
}

ConfirmationQueueItemInfo FromMojomRow(
    const mojom::DBRowInfoPtr& mojom_db_row) {
  CHECK(mojom_db_row);

  ConfirmationQueueItemInfo confirmation_queue_item;

  confirmation_queue_item.confirmation.transaction_id =
      ColumnString(mojom_db_row, 0);

  confirmation_queue_item.confirmation.creative_instance_id =
      ColumnString(mojom_db_row, 1);

  confirmation_queue_item.confirmation.type =
      ToMojomConfirmationType(ColumnString(mojom_db_row, 2));

  confirmation_queue_item.confirmation.ad_type =
      ToMojomAdType(ColumnString(mojom_db_row, 3));

  const base::Time created_at = ColumnTime(mojom_db_row, 4);
  if (!created_at.is_null()) {
    confirmation_queue_item.confirmation.created_at = created_at;
  }

  const std::string token = ColumnString(mojom_db_row, 5);
  const std::string blinded_token = ColumnString(mojom_db_row, 6);
  const std::string unblinded_token = ColumnString(mojom_db_row, 7);
  const std::string public_key = ColumnString(mojom_db_row, 8);
  const std::string signature = ColumnString(mojom_db_row, 9);
  const std::string credential_base64url = ColumnString(mojom_db_row, 10);
  if (!token.empty() && !blinded_token.empty() && !unblinded_token.empty() &&
      !public_key.empty() && !signature.empty() &&
      !credential_base64url.empty()) {
    confirmation_queue_item.confirmation.reward = RewardInfo();

    confirmation_queue_item.confirmation.reward->token = cbr::Token(token);

    confirmation_queue_item.confirmation.reward->blinded_token =
        cbr::BlindedToken(blinded_token);

    confirmation_queue_item.confirmation.reward->unblinded_token =
        cbr::UnblindedToken(unblinded_token);

    confirmation_queue_item.confirmation.reward->public_key =
        cbr::PublicKey(public_key);

    confirmation_queue_item.confirmation.reward->signature = signature;

    confirmation_queue_item.confirmation.reward->credential_base64url =
        credential_base64url;
  }

  confirmation_queue_item.confirmation.user_data.fixed =
      base::JSONReader::ReadDict(ColumnString(mojom_db_row, 11))
          .value_or(base::Value::Dict());

  const base::Time process_at = ColumnTime(mojom_db_row, 12);
  if (!process_at.is_null()) {
    confirmation_queue_item.process_at = process_at;
  }

  confirmation_queue_item.retry_count = ColumnInt(mojom_db_row, 13);

  return confirmation_queue_item;
}

void GetCallback(
    GetConfirmationQueueCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (IsError(mojom_db_transaction_result)) {
    BLOG(0, "Failed to get confirmation queue");
    return std::move(callback).Run(/*success=*/false,
                                   /*confirmations_queue_items=*/{});
  }

  CHECK(mojom_db_transaction_result->rows_union);

  ConfirmationQueueItemList confirmation_queue_items;

  for (const auto& mojom_db_row :
       mojom_db_transaction_result->rows_union->get_rows()) {
    const ConfirmationQueueItemInfo confirmation_queue_item =
        FromMojomRow(mojom_db_row);
    if (!confirmation_queue_item.IsValid()) {
      BLOG(0, "Invalid confirmation queue item");

      continue;
    }

    confirmation_queue_items.push_back(confirmation_queue_item);
  }

  std::move(callback).Run(/*success=*/true, confirmation_queue_items);
}

void MigrateToV36(const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE confirmation_queue (
        id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
        transaction_id TEXT NOT NULL,
        creative_instance_id TEXT NOT NULL,
        type TEXT NOT NULL,
        ad_type TEXT NOT NULL,
        created_at TIMESTAMP NOT NULL,
        token TEXT,
        blinded_token TEXT,
        unblinded_token TEXT,
        public_key TEXT,
        signature TEXT,
        credential_base64url TEXT,
        user_data TEXT NOT NULL,
        process_at TIMESTAMP NOT NULL,
        retry_count INTEGER DEFAULT 0
      );)");

  // Optimize database query for `GetAll, and `GetNext`.
  CreateTableIndex(mojom_db_transaction, /*table_name=*/"confirmation_queue",
                   /*columns=*/{"process_at"});
}

void MigrateToV38(const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // The conversion queue is deprecated since all confirmations are now being
  // added to the confirmation queue.
  DropTable(mojom_db_transaction, "conversion_queue");
}

void MigrateToV43(const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // Optimize database query for `Delete`, and `Retry`.
  CreateTableIndex(mojom_db_transaction, /*table_name=*/"confirmation_queue",
                   /*columns=*/{"transaction_id"});
}

}  // namespace

ConfirmationQueue::ConfirmationQueue() : batch_size_(kDefaultBatchSize) {}

void ConfirmationQueue::Save(
    const ConfirmationQueueItemList& confirmation_queue_items,
    ResultCallback callback) const {
  if (confirmation_queue_items.empty()) {
    return std::move(callback).Run(/*success=*/true);
  }

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  const std::vector<ConfirmationQueueItemList> batches =
      SplitVector(confirmation_queue_items, batch_size_);

  for (const auto& batch : batches) {
    Insert(mojom_db_transaction, batch);
  }

  RunDBTransaction(FROM_HERE, std::move(mojom_db_transaction),
                   std::move(callback));
}

void ConfirmationQueue::DeleteAll(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  DeleteTable(mojom_db_transaction, GetTableName());

  RunDBTransaction(FROM_HERE, std::move(mojom_db_transaction),
                   std::move(callback));
}

void ConfirmationQueue::Delete(const std::string& transaction_id,
                               ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  Execute(mojom_db_transaction, R"(
              DELETE FROM
                $1
              WHERE
                transaction_id = '$2';
            )",
          {GetTableName(), transaction_id});

  RunDBTransaction(FROM_HERE, std::move(mojom_db_transaction),
                   std::move(callback));
}

void ConfirmationQueue::Retry(const std::string& transaction_id,
                              ResultCallback callback) const {
  const std::string retry_after =
      base::NumberToString(RetryProcessingConfirmationAfter().InMicroseconds());

  const std::string max_retry_delay =
      base::NumberToString(kMaximumRetryDelay.InMicroseconds());

  // Exponentially backoff `process_at` for the next retry up to
  // `kMaximumRetryDelay`.
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  Execute(
      mojom_db_transaction, R"(
            UPDATE
              $1
            SET
              retry_count = retry_count + 1,
              process_at = $2 + (
                CASE
                  WHEN ($3 << retry_count) < $4
                  THEN ($5 << retry_count)
                  ELSE $6
                END
              )
            WHERE
              transaction_id = '$7';)",
      {GetTableName(), TimeToSqlValueAsString(base::Time::Now()), retry_after,
       max_retry_delay, retry_after, max_retry_delay, transaction_id});

  RunDBTransaction(FROM_HERE, std::move(mojom_db_transaction),
                   std::move(callback));
}

void ConfirmationQueue::GetAll(GetConfirmationQueueCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kStepStatement;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            transaction_id,
            creative_instance_id,
            type,
            ad_type,
            created_at,
            token,
            blinded_token,
            unblinded_token,
            public_key,
            signature,
            credential_base64url,
            user_data,
            process_at,
            retry_count
          FROM
            $1
          ORDER BY
            process_at ASC;)",
      {GetTableName()}, nullptr);
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunDBTransaction(FROM_HERE, std::move(mojom_db_transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void ConfirmationQueue::GetNext(GetConfirmationQueueCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kStepStatement;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            transaction_id,
            creative_instance_id,
            type,
            ad_type,
            created_at,
            token,
            blinded_token,
            unblinded_token,
            public_key,
            signature,
            credential_base64url,
            user_data,
            process_at,
            retry_count
          FROM
            $1
          ORDER BY
            process_at ASC
          LIMIT
            1;)",
      {GetTableName()}, nullptr);
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunDBTransaction(FROM_HERE, std::move(mojom_db_transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

std::string ConfirmationQueue::GetTableName() const {
  return kTableName;
}

void ConfirmationQueue::Create(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE confirmation_queue (
        id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
        transaction_id TEXT NOT NULL,
        creative_instance_id TEXT NOT NULL,
        type TEXT NOT NULL,
        ad_type TEXT NOT NULL,
        created_at TIMESTAMP NOT NULL,
        token TEXT,
        blinded_token TEXT,
        unblinded_token TEXT,
        public_key TEXT,
        signature TEXT,
        credential_base64url TEXT,
        user_data TEXT NOT NULL,
        process_at TIMESTAMP NOT NULL,
        retry_count INTEGER DEFAULT 0
      );)");

  // Optimize database query for `GetAll, and `GetNext` from schema 36.
  CreateTableIndex(mojom_db_transaction, /*table_name=*/"confirmation_queue",
                   /*columns=*/{"process_at"});

  // Optimize database query for `Delete`, and `Retry` from schema 43.
  CreateTableIndex(mojom_db_transaction, /*table_name=*/"confirmation_queue",
                   /*columns=*/{"transaction_id"});
}

void ConfirmationQueue::Migrate(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    int to_version) {
  CHECK(mojom_db_transaction);

  switch (to_version) {
    case 36: {
      MigrateToV36(mojom_db_transaction);
      break;
    }

    case 38: {
      MigrateToV38(mojom_db_transaction);
      break;
    }

    case 43: {
      MigrateToV43(mojom_db_transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void ConfirmationQueue::Insert(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    const ConfirmationQueueItemList& confirmation_queue_items) const {
  CHECK(mojom_db_transaction);

  if (confirmation_queue_items.empty()) {
    return;
  }

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kRunStatement;
  mojom_db_action->sql =
      BuildInsertSql(mojom_db_action, confirmation_queue_items);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

std::string ConfirmationQueue::BuildInsertSql(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const ConfirmationQueueItemList& confirmation_queue_items) const {
  CHECK(mojom_db_action);
  CHECK(!confirmation_queue_items.empty());

  const size_t row_count =
      BindColumns(mojom_db_action, confirmation_queue_items);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            transaction_id,
            creative_instance_id,
            type,
            ad_type,
            created_at,
            token,
            blinded_token,
            unblinded_token,
            public_key,
            signature,
            credential_base64url,
            user_data,
            process_at,
            retry_count
          ) VALUES $2;)",
      {GetTableName(),
       BuildBindColumnPlaceholders(/*column_count=*/14, row_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
