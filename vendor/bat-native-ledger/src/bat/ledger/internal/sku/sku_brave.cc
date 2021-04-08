/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/sku/sku_brave.h"
#include "bat/ledger/internal/sku/sku_util.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ledger {
namespace sku {

SKUBrave::SKUBrave(LedgerImpl* ledger) :
    ledger_(ledger),
    common_(std::make_unique<SKUCommon>(ledger)) {
  DCHECK(ledger_);
}

SKUBrave::~SKUBrave() = default;

void SKUBrave::Process(
    std::vector<mojom::SKUOrderItemPtr> items,
    const std::string& wallet_type,
    ledger::SKUOrderCallback callback,
    const std::string& contribution_id) {
  auto create_callback = std::bind(&SKUBrave::OrderCreated,
      this,
      _1,
      _2,
      wallet_type,
      contribution_id,
      callback);

  common_->CreateOrder(std::move(items), create_callback);
}

void SKUBrave::OrderCreated(
    const type::Result result,
    const std::string& order_id,
    const std::string& wallet_type,
    const std::string& contribution_id,
    ledger::SKUOrderCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Order was not successful");
    callback(result, "");
    return;
  }

  auto save_callback = std::bind(&SKUBrave::ContributionIdSaved,
      this,
      _1,
      order_id,
      wallet_type,
      callback);

  ledger_->database()->SaveContributionIdForSKUOrder(
      order_id,
      contribution_id,
      save_callback);
}

void SKUBrave::ContributionIdSaved(
    const type::Result result,
    const std::string& order_id,
    const std::string& wallet_type,
    ledger::SKUOrderCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Contribution id not saved");
    callback(result, "");
    return;
  }

  auto get_callback = std::bind(&SKUBrave::CreateTransaction,
      this,
      _1,
      wallet_type,
      callback);

  ledger_->database()->GetSKUOrder(order_id, get_callback);
}

void SKUBrave::CreateTransaction(
    type::SKUOrderPtr order,
    const std::string& wallet_type,
    ledger::SKUOrderCallback callback) {
  if (!order) {
    BLOG(0, "Order not found");
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  const std::string destination = GetBraveDestination(wallet_type);

  common_->CreateTransaction(
      std::move(order),
      destination,
      wallet_type,
      callback);
}

void SKUBrave::Retry(
    const std::string& order_id,
    const std::string& wallet_type,
    ledger::SKUOrderCallback callback) {
  if (order_id.empty()) {
    BLOG(0, "Order id is empty");
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  auto get_callback = std::bind(&SKUBrave::OnOrder,
      this,
      _1,
      wallet_type,
      callback);

  ledger_->database()->GetSKUOrder(order_id, get_callback);
}

void SKUBrave::OnOrder(
    type::SKUOrderPtr order,
    const std::string& wallet_type,
    ledger::SKUOrderCallback callback) {
  if (!order) {
    BLOG(0, "Order is null");
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  switch (order->status) {
    case type::SKUOrderStatus::PENDING: {
      ContributionIdSaved(
          type::Result::LEDGER_OK,
          order->order_id,
          wallet_type,
          callback);
      return;
    }
    case type::SKUOrderStatus::PAID: {
      common_->SendExternalTransaction(order->order_id, callback);
      return;
    }
    case type::SKUOrderStatus::FULFILLED: {
      callback(type::Result::LEDGER_OK, order->order_id);
      return;
    }
    case type::SKUOrderStatus::CANCELED:
    case type::SKUOrderStatus::NONE: {
      callback(type::Result::LEDGER_ERROR, "");
      return;
    }
  }
}

}  // namespace sku
}  // namespace ledger
