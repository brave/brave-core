/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens_database_table.h"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "payment_tokens";

void BindColumnTypes(const mojom::DBActionInfoPtr& mojom_db_action) {
  CHECK(mojom_db_action);

  mojom_db_action->bind_column_types = {
      mojom::DBBindColumnType::kString,  // transaction_id
      mojom::DBBindColumnType::kString,  // unblinded_token
      mojom::DBBindColumnType::kString,  // public_key
      mojom::DBBindColumnType::kString,  // confirmation_type
      mojom::DBBindColumnType::kString   // ad_type
  };
}

size_t BindColumns(const mojom::DBActionInfoPtr& mojom_db_action,
                   const PaymentTokenList& payment_tokens) {
  CHECK(mojom_db_action);
  CHECK(!payment_tokens.empty());

  size_t row_count = 0;

  int32_t index = 0;
  for (const auto& payment_token : payment_tokens) {
    BindColumnString(mojom_db_action, index++, payment_token.transaction_id);
    BindColumnString(mojom_db_action, index++,
                     payment_token.unblinded_token.EncodeBase64().value_or(""));
    BindColumnString(mojom_db_action, index++,
                     payment_token.public_key.EncodeBase64().value_or(""));
    BindColumnString(mojom_db_action, index++,
                     ToString(payment_token.confirmation_type));
    BindColumnString(mojom_db_action, index++, ToString(payment_token.ad_type));

    ++row_count;
  }

  return row_count;
}

PaymentTokenInfo FromMojomRow(const mojom::DBRowInfoPtr& mojom_db_row) {
  CHECK(mojom_db_row);

  PaymentTokenInfo payment_token;
  payment_token.transaction_id = ColumnString(mojom_db_row, 0);
  payment_token.unblinded_token =
      cbr::UnblindedToken(ColumnString(mojom_db_row, 1));
  payment_token.public_key = cbr::PublicKey(ColumnString(mojom_db_row, 2));
  payment_token.confirmation_type =
      ToMojomConfirmationType(ColumnString(mojom_db_row, 3));
  payment_token.ad_type = ToMojomAdType(ColumnString(mojom_db_row, 4));
  return payment_token;
}

void GetAllCallback(
    GetPaymentTokensCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (!IsTransactionSuccessful(mojom_db_transaction_result)) {
    BLOG(0, "Failed to get payment tokens");
    return std::move(callback).Run(/*success=*/false, /*payment_tokens=*/{});
  }

  CHECK(mojom_db_transaction_result->rows_union);

  PaymentTokenList payment_tokens;
  for (auto& mojom_db_row :
       mojom_db_transaction_result->rows_union->get_rows()) {
    payment_tokens.push_back(FromMojomRow(mojom_db_row));
  }

  std::move(callback).Run(/*success=*/true, std::move(payment_tokens));
}

}  // namespace

void PaymentTokens::Save(const PaymentTokenList& payment_tokens,
                         ResultCallback callback) {
  if (payment_tokens.empty()) {
    return std::move(callback).Run(/*success=*/true);
  }

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  Insert(mojom_db_transaction, payment_tokens);

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 std::move(callback));
}

void PaymentTokens::Insert(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    const PaymentTokenList& payment_tokens) {
  CHECK(mojom_db_transaction);

  if (payment_tokens.empty()) {
    return;
  }

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteWithBindings;
  mojom_db_action->sql = BuildInsertSql(mojom_db_action, payment_tokens);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

void PaymentTokens::Delete(const PaymentTokenInfo& payment_token,
                           ResultCallback callback) {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  Execute(mojom_db_transaction, R"(
      DELETE FROM
        $1
      WHERE
        transaction_id = '$2')",
          {kTableName, payment_token.transaction_id});

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 std::move(callback));
}

void PaymentTokens::Delete(const PaymentTokenList& payment_tokens,
                           ResultCallback callback) {
  if (payment_tokens.empty()) {
    return std::move(callback).Run(/*success=*/true);
  }

  std::vector<std::string> transaction_ids;
  transaction_ids.reserve(payment_tokens.size());
  for (const auto& payment_token : payment_tokens) {
    transaction_ids.push_back("'" + payment_token.transaction_id + "'");
  }

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  Execute(mojom_db_transaction, R"(
      DELETE FROM
        $1
      WHERE
        transaction_id IN ($2))",
          {kTableName, base::JoinString(transaction_ids, ", ")});

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 std::move(callback));
}

void PaymentTokens::DeleteAll(ResultCallback callback) {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  Execute(mojom_db_transaction, R"(
      DELETE FROM
        $1)",
          {kTableName});

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 std::move(callback));
}

void PaymentTokens::GetAll(GetPaymentTokensCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteQueryWithBindings;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            transaction_id,
            unblinded_token,
            public_key,
            confirmation_type,
            ad_type
          FROM
            $1)",
      {kTableName}, nullptr);
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 base::BindOnce(&GetAllCallback, std::move(callback)));
}

void PaymentTokens::Create(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE payment_tokens (
        transaction_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        unblinded_token TEXT NOT NULL,
        public_key TEXT NOT NULL,
        confirmation_type TEXT NOT NULL,
        ad_type TEXT NOT NULL
      ))");
}

void PaymentTokens::Migrate(
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

std::string PaymentTokens::BuildInsertSql(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const PaymentTokenList& payment_tokens) const {
  CHECK(mojom_db_action);
  CHECK(!payment_tokens.empty());

  const size_t row_count = BindColumns(mojom_db_action, payment_tokens);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            transaction_id,
            unblinded_token,
            public_key,
            confirmation_type,
            ad_type
          ) VALUES $2)",
      {kTableName, BuildBindColumnPlaceholders(/*column_count=*/5U, row_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
