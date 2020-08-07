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
#include "bat/ledger/internal/request/request_sku.h"
#include "bat/ledger/internal/response/response_sku.h"
#include "bat/ledger/internal/sku/sku_transaction.h"
#include "bat/ledger/internal/sku/sku_util.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace {

ledger::SKUTransactionType GetTransactionTypeFromWalletType(
    const std::string& wallet_type) {
  if (wallet_type == ledger::kWalletUphold) {
    return ledger::SKUTransactionType::UPHOLD;
  }

  if (wallet_type == ledger::kWalletAnonymous) {
    return ledger::SKUTransactionType::ANONYMOUS_CARD;
  }

  if (wallet_type == ledger::kWalletUnBlinded) {
    return ledger::SKUTransactionType::TOKENS;
  }

  NOTREACHED();
  return ledger::SKUTransactionType::ANONYMOUS_CARD;
}

}  // namespace

namespace braveledger_sku {

SKUTransaction::SKUTransaction(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
  DCHECK(ledger_);
}

SKUTransaction::~SKUTransaction() = default;

void SKUTransaction::Create(
    ledger::SKUOrderPtr order,
    const std::string& destination,
    const ledger::ExternalWallet& wallet,
    ledger::ResultCallback callback) {
  if (!order) {
    BLOG(0, "Order is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::SKUTransaction::New();
  transaction->transaction_id = base::GenerateGUID();
  transaction->order_id = order->order_id;
  transaction->type = GetTransactionTypeFromWalletType(wallet.type);
  transaction->amount = order->total_amount;
  transaction->status = ledger::SKUTransactionStatus::CREATED;

  auto save_callback = std::bind(&SKUTransaction::OnTransactionSaved,
      this,
      _1,
      *transaction,
      destination,
      wallet,
      callback);

  ledger_->database()->SaveSKUTransaction(transaction->Clone(), save_callback);
}

void SKUTransaction::OnTransactionSaved(
    const ledger::Result result,
    const ledger::SKUTransaction& transaction,
    const std::string& destination,
    const ledger::ExternalWallet& wallet,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
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
      ledger::ExternalWallet::New(wallet),
      transfer_callback);
}

void SKUTransaction::OnTransfer(
    const ledger::Result result,
    const std::string& external_transaction_id,
    const ledger::SKUTransaction& transaction,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Transaction for order failed " << transaction.order_id);
    callback(result);
    return;
  }

  if (external_transaction_id.empty()) {
    callback(ledger::Result::LEDGER_OK);
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
    const ledger::Result result,
    const ledger::SKUTransaction& transaction,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
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
      ledger::SKUOrderStatus::PAID,
      save_callback);
}

void SKUTransaction::SendExternalTransaction(
    const ledger::Result result,
    const ledger::SKUTransaction& transaction,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Order status not updated");
    callback(ledger::Result::RETRY);
    return;
  }

  // we only want to report external transaction id when we have it
  // we don't have it for all transactions
  if (transaction.external_transaction_id.empty()) {
    BLOG(0, "External transaction id is empty for transaction id "
        << transaction.transaction_id);
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  base::Value body(base::Value::Type::DICTIONARY);
  body.SetStringKey(
      "externalTransactionId",
      transaction.external_transaction_id);
  body.SetStringKey(
      "kind",
      ConvertTransactionTypeToString(transaction.type));

  std::string json;
  base::JSONWriter::Write(body, &json);

  auto url_callback = std::bind(&SKUTransaction::OnSendExternalTransaction,
      this,
      _1,
      callback);

  const std::string url = braveledger_request_util::GetCreateTransactionURL(
      transaction.order_id,
      transaction.type);

  ledger_->LoadURL(
      url,
      {},
      json,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void SKUTransaction::OnSendExternalTransaction(
    const ledger::UrlResponse& response,
    ledger::ResultCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  const ledger::Result result =
      braveledger_response_util::CheckSendExternalTransaction(response);
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "External transaction not sent");
    callback(ledger::Result::RETRY);
    return;
  }

  callback(ledger::Result::LEDGER_OK);
}

}  // namespace braveledger_sku
