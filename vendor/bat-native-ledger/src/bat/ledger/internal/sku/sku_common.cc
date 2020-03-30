/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/sku/sku_common.h"

using std::placeholders::_1;

namespace braveledger_sku {

SKUCommon::SKUCommon(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger),
    order_(std::make_unique<SKUOrder>(ledger)),
    transaction_(std::make_unique<SKUTransaction>(ledger)) {
  DCHECK(ledger_);
}

SKUCommon::~SKUCommon() = default;

void SKUCommon::CreateOrder(
    const std::vector<ledger::SKUOrderItem>& items,
    ledger::SKUOrderCallback callback) {
  order_->Create(items, callback);
}

void SKUCommon::CreateTransaction(
    ledger::SKUOrderPtr order,
    const std::string& destination,
    const ledger::ExternalWallet& wallet,
    ledger::SKUOrderCallback callback) {
  if (!order) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Order not found";
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  auto create_callback = std::bind(&SKUCommon::OnTransactionCompleted,
      this,
      _1,
      order->order_id,
      callback);

  transaction_->Create(
      order->Clone(),
      destination,
      wallet,
      create_callback);
}

void SKUCommon::OnTransactionCompleted(
    const ledger::Result result,
    const std::string& order_id,
    ledger::SKUOrderCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
        << "Order status was not updated";
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  callback(ledger::Result::LEDGER_OK, order_id);
}

void SKUCommon::SendExternalTransaction(
    const std::string& order_id,
    ledger::SKUOrderCallback callback) {
  auto get_callback = std::bind(&SKUCommon::GetSKUTransactionByOrderId,
      this,
      _1,
      callback);

  ledger_->GetSKUTransactionByOrderId(order_id, get_callback);
}

void SKUCommon::GetSKUTransactionByOrderId(
    ledger::SKUTransactionPtr transaction,
    ledger::SKUOrderCallback callback) {
  if (!transaction) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
        << "Transaction is null";
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  auto create_callback = std::bind(&SKUCommon::OnTransactionCompleted,
    this,
    _1,
    transaction->order_id,
    callback);

  transaction_->SendExternalTransaction(
      ledger::Result::LEDGER_OK,
      *transaction,
      create_callback);
}

}  // namespace braveledger_sku
