/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/sku/sku_common.h"

using std::placeholders::_1;

namespace ledger {
namespace sku {

SKUCommon::SKUCommon(LedgerImpl* ledger) :
    ledger_(ledger),
    order_(std::make_unique<SKUOrder>(ledger)),
    transaction_(std::make_unique<SKUTransaction>(ledger)) {
  DCHECK(ledger_);
}

SKUCommon::~SKUCommon() = default;

void SKUCommon::CreateOrder(
    std::vector<mojom::SKUOrderItemPtr> items,
    ledger::SKUOrderCallback callback) {
  order_->Create(std::move(items), callback);
}

void SKUCommon::CreateTransaction(
    type::SKUOrderPtr order,
    const std::string& destination,
    const std::string& wallet_type,
    ledger::SKUOrderCallback callback) {
  if (!order) {
    BLOG(0, "Order not found");
    callback(type::Result::LEDGER_ERROR, "");
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
      wallet_type,
      create_callback);
}

void SKUCommon::OnTransactionCompleted(
    const type::Result result,
    const std::string& order_id,
    ledger::SKUOrderCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Order status was not updated");
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  callback(type::Result::LEDGER_OK, order_id);
}

void SKUCommon::SendExternalTransaction(
    const std::string& order_id,
    ledger::SKUOrderCallback callback) {
  if (order_id.empty()) {
    BLOG(0, "Order id is empty");
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  auto get_callback = std::bind(&SKUCommon::GetSKUTransactionByOrderId,
      this,
      _1,
      callback);

  ledger_->database()->GetSKUTransactionByOrderId(order_id, get_callback);
}

void SKUCommon::GetSKUTransactionByOrderId(
    type::SKUTransactionPtr transaction,
    ledger::SKUOrderCallback callback) {
  if (!transaction) {
    BLOG(0, "Transaction is null");
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  auto create_callback = std::bind(&SKUCommon::OnTransactionCompleted,
    this,
    _1,
    transaction->order_id,
    callback);

  transaction_->SendExternalTransaction(
      type::Result::LEDGER_OK,
      *transaction,
      create_callback);
}

}  // namespace sku
}  // namespace ledger
