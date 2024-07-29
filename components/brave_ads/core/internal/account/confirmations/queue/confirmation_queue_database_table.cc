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
#include "brave/components/brave_ads/core/internal/common/database/database_bind_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
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

void BindRecords(mojom::DBCommandInfo* const command) {
  CHECK(command);

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // transaction_id
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // creative_instance_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // type
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // ad_type
      mojom::DBCommandInfo::RecordBindingType::INT64_TYPE,   // created_at
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // token
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // blinded_token
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // unblinded_token
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // public_key
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // signature
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // credential_base64url
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // user_data
      mojom::DBCommandInfo::RecordBindingType::INT64_TYPE,   // process_at
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE      // retry_count
  };
}

size_t BindParameters(
    mojom::DBCommandInfo* command,
    const ConfirmationQueueItemList& confirmation_queue_items) {
  CHECK(command);

  size_t count = 0;

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

    BindString(command, index++, confirmation.transaction_id);

    BindString(command, index++, confirmation.creative_instance_id);

    BindString(command, index++, ToString(confirmation.type));

    BindString(command, index++, ToString(confirmation.ad_type));

    BindInt64(command, index++,
              ToChromeTimestampFromTime(
                  confirmation.created_at.value_or(base::Time())));

    if (confirmation.reward) {
      BindString(command, index++,
                 confirmation.reward->token.EncodeBase64().value_or(""));

      BindString(
          command, index++,
          confirmation.reward->blinded_token.EncodeBase64().value_or(""));

      BindString(
          command, index++,
          confirmation.reward->unblinded_token.EncodeBase64().value_or(""));

      BindString(command, index++,
                 confirmation.reward->public_key.EncodeBase64().value_or(""));

      BindString(command, index++, confirmation.reward->signature);

      BindString(command, index++, confirmation.reward->credential_base64url);
    } else {
      BindString(command, index++, "");
      BindString(command, index++, "");
      BindString(command, index++, "");
      BindString(command, index++, "");
      BindString(command, index++, "");
      BindString(command, index++, "");
    }

    std::string user_data_json;
    CHECK(
        base::JSONWriter::Write(confirmation.user_data.fixed, &user_data_json));
    BindString(command, index++, user_data_json);

    BindInt64(command, index++,
              ToChromeTimestampFromTime(
                  confirmation_queue_item.process_at.value_or(base::Time())));

    BindInt(command, index++, confirmation_queue_item.retry_count);

    ++count;
  }

  return count;
}

ConfirmationQueueItemInfo GetFromRecord(mojom::DBRecordInfo* const record) {
  CHECK(record);

  ConfirmationQueueItemInfo confirmation_queue_item;

  confirmation_queue_item.confirmation.transaction_id = ColumnString(record, 0);

  confirmation_queue_item.confirmation.creative_instance_id =
      ColumnString(record, 1);

  confirmation_queue_item.confirmation.type =
      ToConfirmationType(ColumnString(record, 2));

  confirmation_queue_item.confirmation.ad_type =
      ToAdType(ColumnString(record, 3));

  const base::Time created_at =
      ToTimeFromChromeTimestamp(ColumnInt64(record, 4));
  if (!created_at.is_null()) {
    confirmation_queue_item.confirmation.created_at = created_at;
  }

  const std::string token = ColumnString(record, 5);
  const std::string blinded_token = ColumnString(record, 6);
  const std::string unblinded_token = ColumnString(record, 7);
  const std::string public_key = ColumnString(record, 8);
  const std::string signature = ColumnString(record, 9);
  const std::string credential_base64url = ColumnString(record, 10);
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
      base::JSONReader::ReadDict(ColumnString(record, 11))
          .value_or(base::Value::Dict());

  const base::Time process_at =
      ToTimeFromChromeTimestamp(ColumnInt64(record, 12));
  if (!process_at.is_null()) {
    confirmation_queue_item.process_at = process_at;
  }

  confirmation_queue_item.retry_count = ColumnInt(record, 13);

  return confirmation_queue_item;
}

void GetCallback(GetConfirmationQueueCallback callback,
                 mojom::DBCommandResponseInfoPtr command_response) {
  if (!command_response ||
      command_response->status !=
          mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get confirmation queue");

    return std::move(callback).Run(/*success=*/false,
                                   /*confirmations_queue_items=*/{});
  }

  CHECK(command_response->result);

  ConfirmationQueueItemList confirmation_queue_items;

  for (const auto& record : command_response->result->get_records()) {
    const ConfirmationQueueItemInfo confirmation_queue_item =
        GetFromRecord(&*record);
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

void MigrateToV36(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
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
          );)";
  transaction->commands.push_back(std::move(command));

  // Optimize database query for `GetNext`.
  CreateTableIndex(transaction, /*table_name=*/"confirmation_queue",
                   /*columns=*/{"process_at"});
}

void MigrateToV38(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  // The conversion queue is deprecated since all confirmations are now being
  // added to the confirmation queue.
  DropTable(transaction, "conversion_queue");
}

}  // namespace

ConfirmationQueue::ConfirmationQueue() : batch_size_(kDefaultBatchSize) {}

void ConfirmationQueue::Save(
    const ConfirmationQueueItemList& confirmation_queue_items,
    ResultCallback callback) const {
  if (confirmation_queue_items.empty()) {
    return std::move(callback).Run(/*success=*/true);
  }

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  const std::vector<ConfirmationQueueItemList> batches =
      SplitVector(confirmation_queue_items, batch_size_);

  for (const auto& batch : batches) {
    InsertOrUpdate(&*transaction, batch);
  }

  RunTransaction(std::move(transaction), std::move(callback));
}

void ConfirmationQueue::DeleteAll(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  DeleteTable(&*transaction, GetTableName());

  RunTransaction(std::move(transaction), std::move(callback));
}

void ConfirmationQueue::Delete(const std::string& transaction_id,
                               ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql = base::ReplaceStringPlaceholders(
      R"(
          DELETE FROM
            $1
          WHERE
            transaction_id = '$2';
      )",
      {GetTableName(), transaction_id}, nullptr);
  transaction->commands.push_back(std::move(command));

  RunTransaction(std::move(transaction), std::move(callback));
}

void ConfirmationQueue::Retry(const std::string& transaction_id,
                              ResultCallback callback) const {
  const std::string retry_after =
      base::NumberToString(RetryProcessingConfirmationAfter().InMicroseconds());

  const std::string max_retry_delay =
      base::NumberToString(kMaximumRetryDelay.InMicroseconds());

  // Exponentially backoff `process_at` for the next retry up to
  // `kMaximumRetryDelay`.
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql = base::ReplaceStringPlaceholders(
      R"(
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
      {GetTableName(),
       base::NumberToString(ToChromeTimestampFromTime(base::Time::Now())),
       retry_after, max_retry_delay, retry_after, max_retry_delay,
       transaction_id},
      nullptr);
  transaction->commands.push_back(std::move(command));

  RunTransaction(std::move(transaction), std::move(callback));
}

void ConfirmationQueue::GetAll(GetConfirmationQueueCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
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
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  RunDBTransaction(std::move(transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void ConfirmationQueue::GetNext(GetConfirmationQueueCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
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
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  RunDBTransaction(std::move(transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

std::string ConfirmationQueue::GetTableName() const {
  return kTableName;
}

void ConfirmationQueue::Create(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
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
          );)";
  transaction->commands.push_back(std::move(command));

  // Optimize database query for `GetNext`.
  CreateTableIndex(transaction, GetTableName(), /*columns=*/{"process_at"});
}

void ConfirmationQueue::Migrate(mojom::DBTransactionInfo* const transaction,
                                const int to_version) {
  CHECK(transaction);

  switch (to_version) {
    case 36: {
      MigrateToV36(transaction);
      break;
    }

    case 38: {
      MigrateToV38(transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void ConfirmationQueue::InsertOrUpdate(
    mojom::DBTransactionInfo* transaction,
    const ConfirmationQueueItemList& confirmation_queue_items) const {
  CHECK(transaction);

  if (confirmation_queue_items.empty()) {
    return;
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->sql = BuildInsertOrUpdateSql(&*command, confirmation_queue_items);
  transaction->commands.push_back(std::move(command));
}

std::string ConfirmationQueue::BuildInsertOrUpdateSql(
    mojom::DBCommandInfo* command,
    const ConfirmationQueueItemList& confirmation_queue_items) const {
  CHECK(command);

  const size_t binded_parameters_count =
      BindParameters(command, confirmation_queue_items);

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
      {GetTableName(), BuildBindingParameterPlaceholders(
                           /*parameters_count=*/14, binded_parameters_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
