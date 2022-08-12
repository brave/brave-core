/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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

ledger::type::SKUTransactionType GetTransactionTypeFromWalletType(
    const std::string& wallet_type) {
  if (wallet_type == ledger::constant::kWalletUphold) {
    return ledger::type::SKUTransactionType::UPHOLD;
  }

  if (wallet_type == ledger::constant::kWalletGemini) {
    return ledger::type::SKUTransactionType::GEMINI;
  }

  if (wallet_type == ledger::constant::kWalletUnBlinded) {
    return ledger::type::SKUTransactionType::TOKENS;
  }

  NOTREACHED();
  return ledger::type::SKUTransactionType::TOKENS;
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

void SKUTransaction::Create(type::SKUOrderPtr order,
                            const std::string& destination,
                            const std::string& wallet_type,
                            ledger::LegacyResultCallback callback) {
  if (!order) {
    BLOG(0, "Order is null");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = type::SKUTransaction::New();
  transaction->transaction_id = base::GenerateGUID();
  transaction->order_id = order->order_id;
  transaction->type = GetTransactionTypeFromWalletType(wallet_type);
  transaction->amount = order->total_amount;
  transaction->status = type::SKUTransactionStatus::CREATED;

  auto save_callback = std::bind(&SKUTransaction::OnTransactionSaved,
      this,
      _1,
      *transaction,
      destination,
      wallet_type,
      callback);

  ledger_->database()->SaveSKUTransaction(transaction->Clone(), save_callback);
}

void SKUTransaction::OnTransactionSaved(type::Result result,
                                        const type::SKUTransaction& transaction,
                                        const std::string& destination,
                                        const std::string& wallet_type,
                                        ledger::LegacyResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Transaction was not saved");
    callback(result);
    return;
  }

  auto transfer_callback = std::bind(&SKUTransaction::OnTransfer,
      this,
      _1,
      _2,
      transaction,
      callback);

  ledger_->contribution()->TransferFunds(
      transaction,
      destination,
      wallet_type,
      transfer_callback);
}

void SKUTransaction::OnTransfer(type::Result result,
                                const std::string& external_transaction_id,
                                const type::SKUTransaction& transaction,
                                ledger::LegacyResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Transaction for order failed " << transaction.order_id);
    callback(result);
    return;
  }

  if (external_transaction_id.empty()) {
    callback(type::Result::LEDGER_OK);
    return;
  }

  auto transaction_new = transaction;
  transaction_new.external_transaction_id = external_transaction_id;

  auto save_callback = std::bind(&SKUTransaction::OnSaveSKUExternalTransaction,
      this,
      _1,
      transaction_new,
      callback);

  // We save SKUTransactionStatus::COMPLETED status in this call
  ledger_->database()->SaveSKUExternalTransaction(
      transaction.transaction_id,
      external_transaction_id,
      save_callback);
}

void SKUTransaction::OnSaveSKUExternalTransaction(
    type::Result result,
    const type::SKUTransaction& transaction,
    ledger::LegacyResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
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
      transaction.order_id,
      type::SKUOrderStatus::PAID,
      save_callback);
}

void SKUTransaction::SendExternalTransaction(
    type::Result result,
    const type::SKUTransaction& transaction,
    ledger::LegacyResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Order status not updated");
    callback(type::Result::RETRY);
    return;
  }

  // we only want to report external transaction id when we have it
  // we don't have it for all transactions
  if (transaction.external_transaction_id.empty()) {
    BLOG(0, "External transaction id is empty for transaction id "
        << transaction.transaction_id);
    callback(type::Result::LEDGER_OK);
    return;
  }

  auto url_callback = std::bind(&SKUTransaction::OnSendExternalTransaction,
      this,
      _1,
      callback);

  switch (transaction.type) {
    case type::SKUTransactionType::NONE:
    case type::SKUTransactionType::TOKENS: {
      NOTREACHED();
      return;
    }
    case type::SKUTransactionType::UPHOLD: {
      payment_server_->post_transaction_uphold()->Request(
          transaction,
          url_callback);
      return;
    }
    case type::SKUTransactionType::GEMINI: {
      payment_server_->post_transaction_gemini()->Request(transaction,
                                                          url_callback);
      return;
    }
  }
}

void SKUTransaction::OnSendExternalTransaction(
    type::Result result,
    ledger::LegacyResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "External transaction not sent");
    callback(type::Result::RETRY);
    return;
  }

  callback(type::Result::LEDGER_OK);
}

}  // namespace sku
}  // namespace ledger
