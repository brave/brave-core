/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "base/guid.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/sku/sku_transaction.h"
#include "bat/ledger/internal/sku/sku_util.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace {

ledger::mojom::SKUTransactionType GetTransactionTypeFromWalletType(
    const std::string& wallet_type) {
  if (wallet_type == ledger::constant::kWalletUphold) {
    return ledger::mojom::SKUTransactionType::UPHOLD;
  }

  if (wallet_type == ledger::constant::kWalletGemini) {
    return ledger::mojom::SKUTransactionType::GEMINI;
  }

  if (wallet_type == ledger::constant::kWalletUnBlinded) {
    return ledger::mojom::SKUTransactionType::TOKENS;
  }

  NOTREACHED();
  return ledger::mojom::SKUTransactionType::TOKENS;
}

}  // namespace

namespace ledger {
namespace sku {

SKUTransaction::SKUTransaction(LedgerImpl* ledger) :
    ledger_(ledger),
    payment_server_(std::make_unique<endpoint::PaymentServer>(ledger)) {
  DCHECK(ledger_);
}

SKUTransaction::~SKUTransaction() = default;

void SKUTransaction::Create(mojom::SKUOrderPtr order,
                            const std::string& destination,
                            const std::string& wallet_type,
                            ledger::LegacyResultCallback callback) {
  if (!order) {
    BLOG(0, "Order is null");
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = mojom::SKUTransaction::New();
  transaction->transaction_id = base::GenerateGUID();
  transaction->order_id = order->order_id;
  transaction->type = GetTransactionTypeFromWalletType(wallet_type);
  transaction->amount = order->total_amount;
  transaction->status = mojom::SKUTransactionStatus::CREATED;

  DCHECK(!order->contribution_id.empty());
  auto save_callback =
      std::bind(&SKUTransaction::OnTransactionSaved, this, _1, *transaction,
                destination, wallet_type, order->contribution_id, callback);

  ledger_->database()->SaveSKUTransaction(transaction->Clone(), save_callback);
}

void SKUTransaction::OnTransactionSaved(
    mojom::Result result,
    const mojom::SKUTransaction& transaction,
    const std::string& destination,
    const std::string& wallet_type,
    const std::string& contribution_id,
    ledger::LegacyResultCallback callback) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Transaction was not saved");
    callback(result);
    return;
  }

  auto transfer_callback =
      std::bind(&SKUTransaction::OnTransfer, this, _1, transaction,
                contribution_id, destination, callback);

  ledger_->contribution()->TransferFunds(transaction, destination, wallet_type,
                                         contribution_id, transfer_callback);
}

void SKUTransaction::OnTransfer(mojom::Result result,
                                const mojom::SKUTransaction& transaction,
                                const std::string& contribution_id,
                                const std::string& destination,
                                ledger::LegacyResultCallback callback) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Transaction for order failed " << transaction.order_id);
    callback(result);
    return;
  }

  ledger_->database()->GetExternalTransaction(
      contribution_id, destination,
      base::BindOnce(&SKUTransaction::OnGetExternalTransaction,
                     base::Unretained(this), std::move(callback), transaction));
}

void SKUTransaction::OnGetExternalTransaction(
    ledger::LegacyResultCallback callback,
    mojom::SKUTransaction&& transaction,
    base::expected<mojom::ExternalTransactionPtr,
                   database::GetExternalTransactionError>
        external_transaction) {
  if (!external_transaction.has_value()) {
    return callback(mojom::Result::LEDGER_OK);
  }

  DCHECK(external_transaction.value());

  transaction.external_transaction_id =
      std::move(external_transaction.value()->transaction_id);

  auto save_callback = std::bind(&SKUTransaction::OnSaveSKUExternalTransaction,
                                 this, _1, transaction, std::move(callback));

  // We save SKUTransactionStatus::COMPLETED status in this call
  ledger_->database()->SaveSKUExternalTransaction(
      transaction.transaction_id, transaction.external_transaction_id,
      std::move(save_callback));
}

void SKUTransaction::OnSaveSKUExternalTransaction(
    mojom::Result result,
    const mojom::SKUTransaction& transaction,
    ledger::LegacyResultCallback callback) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "External transaction was not saved");
    callback(result);
    return;
  }

  auto save_callback = std::bind(&SKUTransaction::SendExternalTransaction,
      this,
      _1,
      transaction,
      callback);

  ledger_->database()->UpdateSKUOrderStatus(
      transaction.order_id, mojom::SKUOrderStatus::PAID, save_callback);
}

void SKUTransaction::SendExternalTransaction(
    mojom::Result result,
    const mojom::SKUTransaction& transaction,
    ledger::LegacyResultCallback callback) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Order status not updated");
    callback(mojom::Result::RETRY);
    return;
  }

  // we only want to report external transaction id when we have it
  // we don't have it for all transactions
  if (transaction.external_transaction_id.empty()) {
    BLOG(0, "External transaction id is empty for transaction id "
        << transaction.transaction_id);
    callback(mojom::Result::LEDGER_OK);
    return;
  }

  auto url_callback = std::bind(&SKUTransaction::OnSendExternalTransaction,
      this,
      _1,
      callback);

  switch (transaction.type) {
    case mojom::SKUTransactionType::NONE:
    case mojom::SKUTransactionType::TOKENS: {
      NOTREACHED();
      return;
    }
    case mojom::SKUTransactionType::UPHOLD: {
      payment_server_->post_transaction_uphold()->Request(
          transaction,
          url_callback);
      return;
    }
    case mojom::SKUTransactionType::GEMINI: {
      payment_server_->post_transaction_gemini()->Request(transaction,
                                                          url_callback);
      return;
    }
  }
}

void SKUTransaction::OnSendExternalTransaction(
    mojom::Result result,
    ledger::LegacyResultCallback callback) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "External transaction not sent");
    callback(mojom::Result::RETRY);
    return;
  }

  callback(mojom::Result::LEDGER_OK);
}

}  // namespace sku
}  // namespace ledger
