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
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/sku/sku_transaction.h"

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

  return mojom::SKUTransactionType::TOKENS;
}

}  // namespace

namespace sku {

SKUTransaction::SKUTransaction(RewardsEngine& engine)
    : engine_(engine), payment_server_(engine) {}

SKUTransaction::~SKUTransaction() = default;

void SKUTransaction::Run(mojom::SKUOrderPtr order,
                         const std::string& destination,
                         const std::string& wallet_type,
                         ResultCallback callback) {
  if (!order) {
    engine_->LogError(FROM_HERE) << "Order is null";
    return std::move(callback).Run(mojom::Result::FAILED);
  }

  DCHECK(!order->contribution_id.empty());

  std::string contribution_id = order->contribution_id;

  MaybeCreateTransaction(
      std::move(order), wallet_type,
      base::BindOnce(&SKUTransaction::OnTransactionSaved,
                     weak_factory_.GetWeakPtr(), destination, wallet_type,
                     contribution_id, std::move(callback)));
}

void SKUTransaction::MaybeCreateTransaction(
    mojom::SKUOrderPtr order,
    const std::string& wallet_type,
    MaybeCreateTransactionCallback callback) {
  DCHECK(order);
  engine_->database()->GetSKUTransactionByOrderId(
      order->order_id,
      base::BindOnce(&SKUTransaction::OnGetSKUTransactionByOrderId,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     order->order_id, wallet_type, order->total_amount));
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
    return std::move(callback).Run(mojom::Result::OK, *result.value());
  }

  switch (result.error()) {
    case database::GetSKUTransactionError::kDatabaseError:
      engine_->LogError(FROM_HERE)
          << "Failed to get SKU transaction from database";
      return std::move(callback).Run(mojom::Result::FAILED, {});
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

  auto on_save_sku_transaction = base::BindOnce(
      [](MaybeCreateTransactionCallback callback,
         mojom::SKUTransactionPtr transaction, mojom::Result result) {
        std::move(callback).Run(result, *transaction);
      },
      std::move(callback), transaction->Clone());

  engine_->database()->SaveSKUTransaction(std::move(transaction),
                                          std::move(on_save_sku_transaction));
}

void SKUTransaction::OnTransactionSaved(
    const std::string& destination,
    const std::string& wallet_type,
    const std::string& contribution_id,
    ResultCallback callback,
    mojom::Result result,
    const mojom::SKUTransaction& transaction) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Transaction was not saved";
    std::move(callback).Run(result);
    return;
  }

  engine_->contribution()->TransferFunds(
      transaction, destination, wallet_type, contribution_id,
      base::BindOnce(&SKUTransaction::OnTransfer, weak_factory_.GetWeakPtr(),
                     transaction, contribution_id, destination,
                     std::move(callback)));
}

void SKUTransaction::OnTransfer(const mojom::SKUTransaction& transaction,
                                const std::string& contribution_id,
                                const std::string& destination,
                                ResultCallback callback,
                                mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE)
        << "Transaction for order failed " << transaction.order_id;
    std::move(callback).Run(result);
    return;
  }

  engine_->database()->GetExternalTransaction(
      contribution_id, destination,
      base::BindOnce(&SKUTransaction::OnGetExternalTransaction,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     transaction));
}

void SKUTransaction::OnGetExternalTransaction(
    ResultCallback callback,
    mojom::SKUTransaction&& transaction,
    base::expected<mojom::ExternalTransactionPtr,
                   database::GetExternalTransactionError>
        external_transaction) {
  if (!external_transaction.has_value()) {
    return std::move(callback).Run(mojom::Result::OK);
  }

  DCHECK(external_transaction.value());

  transaction.external_transaction_id =
      std::move(external_transaction.value()->transaction_id);

  // We save SKUTransactionStatus::COMPLETED status in this call
  engine_->database()->SaveSKUExternalTransaction(
      transaction.transaction_id, transaction.external_transaction_id,
      base::BindOnce(&SKUTransaction::OnSaveSKUExternalTransaction,
                     weak_factory_.GetWeakPtr(), transaction,
                     std::move(callback)));
}

void SKUTransaction::OnSaveSKUExternalTransaction(
    const mojom::SKUTransaction& transaction,
    ResultCallback callback,
    mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "External transaction was not saved";
    std::move(callback).Run(result);
    return;
  }

  engine_->database()->UpdateSKUOrderStatus(
      transaction.order_id, mojom::SKUOrderStatus::PAID,
      base::BindOnce(&SKUTransaction::SendExternalTransaction,
                     weak_factory_.GetWeakPtr(), transaction,
                     std::move(callback)));
}

void SKUTransaction::SendExternalTransaction(
    const mojom::SKUTransaction& transaction,
    ResultCallback callback,
    mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Order status not updated";
    std::move(callback).Run(mojom::Result::RETRY);
    return;
  }

  // we only want to report external transaction id when we have it
  // we don't have it for all transactions
  if (transaction.external_transaction_id.empty()) {
    engine_->LogError(FROM_HERE)
        << "External transaction id is empty for transaction id "
        << transaction.transaction_id;
    std::move(callback).Run(mojom::Result::OK);
    return;
  }

  auto url_callback =
      base::BindOnce(&SKUTransaction::OnSendExternalTransaction,
                     weak_factory_.GetWeakPtr(), std::move(callback));

  switch (transaction.type) {
    case mojom::SKUTransactionType::NONE:
    case mojom::SKUTransactionType::TOKENS: {
      std::move(url_callback).Run(mojom::Result::FAILED);
      return;
    }
    case mojom::SKUTransactionType::UPHOLD: {
      payment_server_.post_transaction_uphold().Request(
          transaction, std::move(url_callback));
      return;
    }
    case mojom::SKUTransactionType::GEMINI: {
      payment_server_.post_transaction_gemini().Request(
          transaction, std::move(url_callback));
      return;
    }
  }
}

void SKUTransaction::OnSendExternalTransaction(ResultCallback callback,
                                               mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "External transaction not sent";
    std::move(callback).Run(mojom::Result::RETRY);
    return;
  }

  std::move(callback).Run(mojom::Result::OK);
}

}  // namespace sku
}  // namespace brave_rewards::internal
