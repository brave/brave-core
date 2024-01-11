/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "base/json/json_writer.h"
#include "base/uuid.h"
#include "base/values.h"
#include "brave/components/brave_rewards/core/contribution/contribution.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/sku/sku_transaction.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace brave_rewards::internal {

namespace {

mojom::SKUTransactionType GetTransactionTypeFromWalletType(
    const std::string& wallet_type) {
  if (wallet_type == constant::kWalletUphold) {
    return mojom::SKUTransactionType::UPHOLD;
  }

  if (wallet_type == constant::kWalletGemini) {
    return mojom::SKUTransactionType::GEMINI;
  }

  if (wallet_type == constant::kWalletUnBlinded) {
    return mojom::SKUTransactionType::TOKENS;
  }

  NOTREACHED();
  return mojom::SKUTransactionType::TOKENS;
}

}  // namespace

namespace sku {

SKUTransaction::SKUTransaction(RewardsEngineImpl& engine)
    : engine_(engine), payment_server_(engine) {}

SKUTransaction::~SKUTransaction() = default;

void SKUTransaction::Run(mojom::SKUOrderPtr order,
                         const std::string& destination,
                         const std::string& wallet_type,
                         LegacyResultCallback callback) {
  if (!order) {
    BLOG(0, "Order is null!");
    return callback(mojom::Result::FAILED);
  }

  DCHECK(!order->contribution_id.empty());
  auto on_maybe_create_transaction =
      std::bind(&SKUTransaction::OnTransactionSaved, this, _1, _2, destination,
                wallet_type, order->contribution_id, std::move(callback));

  MaybeCreateTransaction(std::move(order), wallet_type,
                         std::move(on_maybe_create_transaction));
}

void SKUTransaction::MaybeCreateTransaction(
    mojom::SKUOrderPtr order,
    const std::string& wallet_type,
    MaybeCreateTransactionCallback callback) {
  DCHECK(order);
  engine_->database()->GetSKUTransactionByOrderId(
      order->order_id, std::bind(&SKUTransaction::OnGetSKUTransactionByOrderId,
                                 this, std::move(callback), order->order_id,
                                 wallet_type, order->total_amount, _1));
}

void SKUTransaction::OnGetSKUTransactionByOrderId(
    MaybeCreateTransactionCallback callback,
    const std::string& order_id,
    const std::string& wallet_type,
    double total_amount,
    base::expected<mojom::SKUTransactionPtr, database::GetSKUTransactionError>
        result) {
  if (result.has_value()) {
    DCHECK(result.value());
    return callback(mojom::Result::OK, *result.value());
  }

  switch (result.error()) {
    case database::GetSKUTransactionError::kDatabaseError:
      BLOG(0, "Failed to get SKU transaction from database!");
      return callback(mojom::Result::FAILED, {});
    case database::GetSKUTransactionError::kTransactionNotFound:
      break;
  }

  auto transaction = mojom::SKUTransaction::New();
  transaction->transaction_id =
      base::Uuid::GenerateRandomV4().AsLowercaseString();
  transaction->order_id = order_id;
  transaction->type = GetTransactionTypeFromWalletType(wallet_type);
  transaction->amount = total_amount;
  transaction->status = mojom::SKUTransactionStatus::CREATED;

  auto on_save_sku_transaction =
      std::bind(std::move(callback), _1, *transaction);

  engine_->database()->SaveSKUTransaction(std::move(transaction),
                                          std::move(on_save_sku_transaction));
}

void SKUTransaction::OnTransactionSaved(
    mojom::Result result,
    const mojom::SKUTransaction& transaction,
    const std::string& destination,
    const std::string& wallet_type,
    const std::string& contribution_id,
    LegacyResultCallback callback) {
  if (result != mojom::Result::OK) {
    BLOG(0, "Transaction was not saved");
    callback(result);
    return;
  }

  auto transfer_callback =
      std::bind(&SKUTransaction::OnTransfer, this, _1, transaction,
                contribution_id, destination, callback);

  engine_->contribution()->TransferFunds(transaction, destination, wallet_type,
                                         contribution_id, transfer_callback);
}

void SKUTransaction::OnTransfer(mojom::Result result,
                                const mojom::SKUTransaction& transaction,
                                const std::string& contribution_id,
                                const std::string& destination,
                                LegacyResultCallback callback) {
  if (result != mojom::Result::OK) {
    BLOG(0, "Transaction for order failed " << transaction.order_id);
    callback(result);
    return;
  }

  engine_->database()->GetExternalTransaction(
      contribution_id, destination,
      base::BindOnce(&SKUTransaction::OnGetExternalTransaction,
                     base::Unretained(this), std::move(callback), transaction));
}

void SKUTransaction::OnGetExternalTransaction(
    LegacyResultCallback callback,
    mojom::SKUTransaction&& transaction,
    base::expected<mojom::ExternalTransactionPtr,
                   database::GetExternalTransactionError>
        external_transaction) {
  if (!external_transaction.has_value()) {
    return callback(mojom::Result::OK);
  }

  DCHECK(external_transaction.value());

  transaction.external_transaction_id =
      std::move(external_transaction.value()->transaction_id);

  auto save_callback = std::bind(&SKUTransaction::OnSaveSKUExternalTransaction,
                                 this, _1, transaction, std::move(callback));

  // We save SKUTransactionStatus::COMPLETED status in this call
  engine_->database()->SaveSKUExternalTransaction(
      transaction.transaction_id, transaction.external_transaction_id,
      std::move(save_callback));
}

void SKUTransaction::OnSaveSKUExternalTransaction(
    mojom::Result result,
    const mojom::SKUTransaction& transaction,
    LegacyResultCallback callback) {
  if (result != mojom::Result::OK) {
    BLOG(0, "External transaction was not saved");
    callback(result);
    return;
  }

  auto save_callback = std::bind(&SKUTransaction::SendExternalTransaction, this,
                                 _1, transaction, callback);

  engine_->database()->UpdateSKUOrderStatus(
      transaction.order_id, mojom::SKUOrderStatus::PAID, save_callback);
}

void SKUTransaction::SendExternalTransaction(
    mojom::Result result,
    const mojom::SKUTransaction& transaction,
    LegacyResultCallback callback) {
  if (result != mojom::Result::OK) {
    BLOG(0, "Order status not updated");
    callback(mojom::Result::RETRY);
    return;
  }

  // we only want to report external transaction id when we have it
  // we don't have it for all transactions
  if (transaction.external_transaction_id.empty()) {
    BLOG(0, "External transaction id is empty for transaction id "
                << transaction.transaction_id);
    callback(mojom::Result::OK);
    return;
  }

  auto url_callback =
      std::bind(&SKUTransaction::OnSendExternalTransaction, this, _1, callback);

  switch (transaction.type) {
    case mojom::SKUTransactionType::NONE:
    case mojom::SKUTransactionType::TOKENS: {
      NOTREACHED();
      return;
    }
    case mojom::SKUTransactionType::UPHOLD: {
      payment_server_.post_transaction_uphold().Request(transaction,
                                                        url_callback);
      return;
    }
    case mojom::SKUTransactionType::GEMINI: {
      payment_server_.post_transaction_gemini().Request(transaction,
                                                        url_callback);
      return;
    }
  }
}

void SKUTransaction::OnSendExternalTransaction(mojom::Result result,
                                               LegacyResultCallback callback) {
  if (result != mojom::Result::OK) {
    BLOG(0, "External transaction not sent");
    callback(mojom::Result::RETRY);
    return;
  }

  callback(mojom::Result::OK);
}

}  // namespace sku
}  // namespace brave_rewards::internal
