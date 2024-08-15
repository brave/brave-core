/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_database_table.h"

#include <cstddef>
#include <utility>
#include <vector>

#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_feature.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
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

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "confirmation_queue";

constexpr int kDefaultBatchSize = 50;

constexpr base::TimeDelta kMaximumRetryDelay = base::Hours(1);

void BindColumnTypes(mojom::DBStatementInfo* const mojom_statement) {
  CHECK(mojom_statement);

  mojom_statement->bind_column_types = {
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

size_t BindColumns(mojom::DBStatementInfo* mojom_statement,
                   const ConfirmationQueueItemList& confirmation_queue_items) {
  CHECK(mojom_statement);
  CHECK(!confirmation_queue_items.empty());

  size_t row_count = 0;

  int index = 0;
  for (const auto& confirmation_queue_item : confirmation_queue_items) {
    if (!confirmation_queue_item.IsValid()) {
      // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
      // potential defects using `DumpWithoutCrashing`.
      SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                                "Invalid confirmation queue item");
      base::debug::DumpWithoutCrashing();

      BLOG(0, "Invalid confirmation queue item");

      continue;
    }

    // The queue does not store dynamic user data for a confirmation due to the
    // token redemption process which rebuilds the confirmation. Hence, we must
    // regenerate the confirmation without the dynamic user data.
    const ConfirmationInfo confirmation =
        RebuildConfirmationWithoutDynamicUserData(
            confirmation_queue_item.confirmation);

    BindColumnString(mojom_statement, index++, confirmation.transaction_id);

    BindColumnString(mojom_statement, index++,
                     confirmation.creative_instance_id);

    BindColumnString(mojom_statement, index++, ToString(confirmation.type));

    BindColumnString(mojom_statement, index++, ToString(confirmation.ad_type));

    BindColumnTime(mojom_statement, index++,
                   confirmation.created_at.value_or(base::Time()));

    if (confirmation.reward) {
      BindColumnString(mojom_statement, index++,
                       confirmation.reward->token.EncodeBase64().value_or(""));

      BindColumnString(
          mojom_statement, index++,
          confirmation.reward->blinded_token.EncodeBase64().value_or(""));

      BindColumnString(
          mojom_statement, index++,
          confirmation.reward->unblinded_token.EncodeBase64().value_or(""));

      BindColumnString(
          mojom_statement, index++,
          confirmation.reward->public_key.EncodeBase64().value_or(""));

      BindColumnString(mojom_statement, index++,
                       confirmation.reward->signature);

      BindColumnString(mojom_statement, index++,
                       confirmation.reward->credential_base64url);
    } else {
      BindColumnString(mojom_statement, index++, "");
      BindColumnString(mojom_statement, index++, "");
      BindColumnString(mojom_statement, index++, "");
      BindColumnString(mojom_statement, index++, "");
      BindColumnString(mojom_statement, index++, "");
      BindColumnString(mojom_statement, index++, "");
    }

    std::string user_data_json;
    CHECK(
        base::JSONWriter::Write(confirmation.user_data.fixed, &user_data_json));
    BindColumnString(mojom_statement, index++, user_data_json);

    BindColumnTime(mojom_statement, index++,
                   confirmation_queue_item.process_at.value_or(base::Time()));

    BindColumnInt(mojom_statement, index++,
                  confirmation_queue_item.retry_count);

    ++row_count;
  }

  return row_count;
}

ConfirmationQueueItemInfo FromMojomRow(
    const mojom::DBRowInfo* const mojom_row) {
  CHECK(mojom_row);

  ConfirmationQueueItemInfo confirmation_queue_item;

  confirmation_queue_item.confirmation.transaction_id =
      ColumnString(mojom_row, 0);

  confirmation_queue_item.confirmation.creative_instance_id =
      ColumnString(mojom_row, 1);

  confirmation_queue_item.confirmation.type =
      ToConfirmationType(ColumnString(mojom_row, 2));

  confirmation_queue_item.confirmation.ad_type =
      ToAdType(ColumnString(mojom_row, 3));

  const base::Time created_at = ColumnTime(mojom_row, 4);
  if (!created_at.is_null()) {
    confirmation_queue_item.confirmation.created_at = created_at;
  }

  const std::string token = ColumnString(mojom_row, 5);
  const std::string blinded_token = ColumnString(mojom_row, 6);
  const std::string unblinded_token = ColumnString(mojom_row, 7);
  const std::string public_key = ColumnString(mojom_row, 8);
  const std::string signature = ColumnString(mojom_row, 9);
  const std::string credential_base64url = ColumnString(mojom_row, 10);
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
      base::JSONReader::ReadDict(ColumnString(mojom_row, 11))
          .value_or(base::Value::Dict());

  const base::Time process_at = ColumnTime(mojom_row, 12);
  if (!process_at.is_null()) {
    confirmation_queue_item.process_at = process_at;
  }

  confirmation_queue_item.retry_count = ColumnInt(mojom_row, 13);

  return confirmation_queue_item;
}

void GetCallback(GetConfirmationQueueCallback callback,
                 mojom::DBStatementResultInfoPtr mojom_statement_result) {
  if (!mojom_statement_result ||
      mojom_statement_result->result_code !=
          mojom::DBStatementResultInfo::ResultCode::kSuccess) {
    BLOG(0, "Failed to get confirmation queue");

    return std::move(callback).Run(/*success=*/false,
                                   /*confirmations_queue_items=*/{});
  }

  CHECK(mojom_statement_result->rows_union);

  ConfirmationQueueItemList confirmation_queue_items;

  for (const auto& mojom_row : mojom_statement_result->rows_union->get_rows()) {
    const ConfirmationQueueItemInfo confirmation_queue_item =
        FromMojomRow(&*mojom_row);
    if (!confirmation_queue_item.IsValid()) {
      // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
      // potential defects using `DumpWithoutCrashing`.
      SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                                "Invalid confirmation queue item");
      base::debug::DumpWithoutCrashing();

      BLOG(0, "Invalid confirmation queue item");

      continue;
    }

    confirmation_queue_items.push_back(confirmation_queue_item);
  }

  std::move(callback).Run(/*success=*/true, confirmation_queue_items);
}

void MigrateToV36(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  Execute(mojom_transaction, R"(
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
  CreateTableIndex(mojom_transaction, /*table_name=*/"confirmation_queue",
                   /*columns=*/{"process_at"});
}

void MigrateToV38(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // The conversion queue is deprecated since all confirmations are now being
  // added to the confirmation queue.
  DropTable(mojom_transaction, "conversion_queue");
}

void MigrateToV43(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Optimize database query for `Delete`, and `Retry`.
  CreateTableIndex(mojom_transaction, /*table_name=*/"confirmation_queue",
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

  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();

  const std::vector<ConfirmationQueueItemList> batches =
      SplitVector(confirmation_queue_items, batch_size_);

  for (const auto& batch : batches) {
    Insert(&*mojom_transaction, batch);
  }

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

void ConfirmationQueue::DeleteAll(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();

  DeleteTable(&*mojom_transaction, GetTableName());

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

void ConfirmationQueue::Delete(const std::string& transaction_id,
                               ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  Execute(&*mojom_transaction, R"(
              DELETE FROM
                $1
              WHERE
                transaction_id = '$2';
            )",
          {GetTableName(), transaction_id});

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

void ConfirmationQueue::Retry(const std::string& transaction_id,
                              ResultCallback callback) const {
  const std::string retry_after =
      base::NumberToString(RetryProcessingConfirmationAfter().InMicroseconds());

  const std::string max_retry_delay =
      base::NumberToString(kMaximumRetryDelay.InMicroseconds());

  // Exponentially backoff `process_at` for the next retry up to
  // `kMaximumRetryDelay`.
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  Execute(
      &*mojom_transaction, R"(
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

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

void ConfirmationQueue::GetAll(GetConfirmationQueueCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kStep;
  mojom_statement->sql = base::ReplaceStringPlaceholders(
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
  BindColumnTypes(&*mojom_statement);
  mojom_transaction->statements.push_back(std::move(mojom_statement));

  RunDBTransaction(std::move(mojom_transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void ConfirmationQueue::GetNext(GetConfirmationQueueCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kStep;
  mojom_statement->sql = base::ReplaceStringPlaceholders(
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
  BindColumnTypes(&*mojom_statement);
  mojom_transaction->statements.push_back(std::move(mojom_statement));

  RunDBTransaction(std::move(mojom_transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

std::string ConfirmationQueue::GetTableName() const {
  return kTableName;
}

void ConfirmationQueue::Create(
    mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  Execute(mojom_transaction, R"(
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
  CreateTableIndex(mojom_transaction, /*table_name=*/"confirmation_queue",
                   /*columns=*/{"process_at"});

  // Optimize database query for `Delete`, and `Retry` from schema 43.
  CreateTableIndex(mojom_transaction, /*table_name=*/"confirmation_queue",
                   /*columns=*/{"transaction_id"});
}

void ConfirmationQueue::Migrate(
    mojom::DBTransactionInfo* const mojom_transaction,
    const int to_version) {
  CHECK(mojom_transaction);

  switch (to_version) {
    case 36: {
      MigrateToV36(mojom_transaction);
      break;
    }

    case 38: {
      MigrateToV38(mojom_transaction);
      break;
    }

    case 43: {
      MigrateToV43(mojom_transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void ConfirmationQueue::Insert(
    mojom::DBTransactionInfo* mojom_transaction,
    const ConfirmationQueueItemList& confirmation_queue_items) const {
  CHECK(mojom_transaction);

  if (confirmation_queue_items.empty()) {
    return;
  }

  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type = mojom::DBStatementInfo::OperationType::kRun;
  mojom_statement->sql =
      BuildInsertSql(&*mojom_statement, confirmation_queue_items);
  mojom_transaction->statements.push_back(std::move(mojom_statement));
}

std::string ConfirmationQueue::BuildInsertSql(
    mojom::DBStatementInfo* mojom_statement,
    const ConfirmationQueueItemList& confirmation_queue_items) const {
  CHECK(mojom_statement);
  CHECK(!confirmation_queue_items.empty());

  const size_t row_count =
      BindColumns(mojom_statement, confirmation_queue_items);

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
