/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_database_table.h"

#include <cstddef>
#include <cstdint>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_token_info.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "confirmation_tokens";

void BindColumnTypes(const mojom::DBActionInfoPtr& mojom_db_action) {
  CHECK(mojom_db_action);

  mojom_db_action->bind_column_types = {
      mojom::DBBindColumnType::kString,  // unblinded_token
      mojom::DBBindColumnType::kString,  // public_key
      mojom::DBBindColumnType::kString   // signature
  };
}

size_t BindColumns(const mojom::DBActionInfoPtr& mojom_db_action,
                   const ConfirmationTokenList& confirmation_tokens) {
  CHECK(mojom_db_action);
  CHECK(!confirmation_tokens.empty());

  size_t row_count = 0;

  int32_t index = 0;
  for (const auto& confirmation_token : confirmation_tokens) {
    BindColumnString(
        mojom_db_action, index++,
        confirmation_token.unblinded_token.EncodeBase64().value_or(""));
    BindColumnString(mojom_db_action, index++,
                     confirmation_token.public_key.EncodeBase64().value_or(""));
    BindColumnString(mojom_db_action, index++,
                     confirmation_token.signature_base64);

    ++row_count;
  }

  return row_count;
}

ConfirmationTokenInfo FromMojomRow(const mojom::DBRowInfoPtr& mojom_db_row) {
  CHECK(mojom_db_row);

  ConfirmationTokenInfo confirmation_token;
  confirmation_token.unblinded_token =
      cbr::UnblindedToken(ColumnString(mojom_db_row, 0));
  confirmation_token.public_key = cbr::PublicKey(ColumnString(mojom_db_row, 1));
  confirmation_token.signature_base64 = ColumnString(mojom_db_row, 2);
  return confirmation_token;
}

void GetAllCallback(
    GetConfirmationTokensCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (!IsTransactionSuccessful(mojom_db_transaction_result)) {
    BLOG(0, "Failed to get confirmation tokens");
    return std::move(callback).Run(/*success=*/false,
                                   /*confirmation_tokens=*/{});
  }

  CHECK(mojom_db_transaction_result->rows_union);

  ConfirmationTokenList confirmation_tokens;
  for (auto& mojom_db_row :
       mojom_db_transaction_result->rows_union->get_rows()) {
    confirmation_tokens.push_back(FromMojomRow(mojom_db_row));
  }

  std::move(callback).Run(/*success=*/true, std::move(confirmation_tokens));
}

}  // namespace

void ConfirmationTokens::Save(const ConfirmationTokenList& confirmation_tokens,
                              ResultCallback callback) {
  if (confirmation_tokens.empty()) {
    return std::move(callback).Run(/*success=*/true);
  }

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  Insert(mojom_db_transaction, confirmation_tokens);

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 std::move(callback));
}

void ConfirmationTokens::Insert(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    const ConfirmationTokenList& confirmation_tokens) {
  CHECK(mojom_db_transaction);

  if (confirmation_tokens.empty()) {
    return;
  }

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteWithBindings;
  mojom_db_action->sql = BuildInsertSql(mojom_db_action, confirmation_tokens);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

void ConfirmationTokens::Delete(const ConfirmationTokenInfo& confirmation_token,
                                ResultCallback callback) {
  const std::optional<std::string> unblinded_token_base64 =
      confirmation_token.unblinded_token.EncodeBase64();
  if (!unblinded_token_base64) {
    BLOG(0, "Failed to delete confirmation token");
    return std::move(callback).Run(/*success=*/false);
  }

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  Execute(mojom_db_transaction, R"(
      DELETE FROM
        $1
      WHERE
        unblinded_token = '$2')",
          {kTableName, *unblinded_token_base64});

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 std::move(callback));
}

void ConfirmationTokens::DeleteAll(ResultCallback callback) {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  Execute(mojom_db_transaction, R"(
      DELETE FROM
        $1)",
          {kTableName});

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 std::move(callback));
}

void ConfirmationTokens::GetAll(GetConfirmationTokensCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteQueryWithBindings;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            unblinded_token,
            public_key,
            signature
          FROM
            $1)",
      {kTableName}, nullptr);
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 base::BindOnce(&GetAllCallback, std::move(callback)));
}

void ConfirmationTokens::Create(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE confirmation_tokens (
        unblinded_token TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        public_key TEXT NOT NULL,
        signature TEXT NOT NULL
      ))");
}

void ConfirmationTokens::Migrate(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    int to_version) {
  CHECK(mojom_db_transaction);

  switch (to_version) {
    case 56: {
      Create(mojom_db_transaction);
      break;
    }

    default: {
      // No migration needed.
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

std::string ConfirmationTokens::BuildInsertSql(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const ConfirmationTokenList& confirmation_tokens) const {
  CHECK(mojom_db_action);
  CHECK(!confirmation_tokens.empty());

  const size_t row_count = BindColumns(mojom_db_action, confirmation_tokens);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            unblinded_token,
            public_key,
            signature
          ) VALUES $2)",
      {kTableName, BuildBindColumnPlaceholders(/*column_count=*/3U, row_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
