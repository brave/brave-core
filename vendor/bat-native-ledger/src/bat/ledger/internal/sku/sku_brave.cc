/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/common/bind_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/sku/sku_brave.h"
#include "bat/ledger/internal/sku/sku_util.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace braveledger_sku {

SKUBrave::SKUBrave(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger),
    common_(std::make_unique<SKUCommon>(ledger)) {
  DCHECK(ledger_);
}

SKUBrave::~SKUBrave() = default;

void SKUBrave::Process(
    const std::vector<ledger::SKUOrderItem>& items,
    ledger::ExternalWalletPtr wallet,
    ledger::SKUOrderCallback callback,
    const std::string& contribution_id) {
  if (!wallet) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Wallet is null";
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  auto create_callback = std::bind(&SKUBrave::OrderCreated,
      this,
      _1,
      _2,
      *wallet,
      contribution_id,
      callback);

  common_->CreateOrder(items, create_callback);
}

void SKUBrave::OrderCreated(
    const ledger::Result result,
    const std::string& order_id,
    const ledger::ExternalWallet& wallet,
    const std::string& contribution_id,
    ledger::SKUOrderCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Order was not successful";
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  auto save_callback = std::bind(&SKUBrave::ContributionIdSaved,
      this,
      _1,
      order_id,
      wallet,
      callback);

  ledger_->SaveContributionIdForSKUOrder(
      order_id,
      contribution_id,
      save_callback);
}

void SKUBrave::ContributionIdSaved(
    const ledger::Result result,
    const std::string& order_id,
    const ledger::ExternalWallet& wallet,
    ledger::SKUOrderCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Contribution id not saved";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback = std::bind(&SKUBrave::CreateTransaction,
      this,
      _1,
      wallet,
      callback);

  ledger_->GetSKUOrder(order_id, get_callback);
}

void SKUBrave::CreateTransaction(
    ledger::SKUOrderPtr order,
    const ledger::ExternalWallet& wallet,
    ledger::SKUOrderCallback callback) {
  if (!order) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Order not found";
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  const std::string destination = GetBraveDestination(wallet.type);

  common_->CreateTransaction(std::move(order), destination, wallet, callback);
}

void SKUBrave::Retry(
    const std::string& order_id,
    ledger::ExternalWalletPtr wallet,
    ledger::SKUOrderCallback callback) {
  auto get_callback = std::bind(&SKUBrave::OnOrder,
      this,
      _1,
      *wallet,
      callback);

  ledger_->GetSKUOrder(order_id, get_callback);
}

void SKUBrave::OnOrder(
    ledger::SKUOrderPtr order,
    const ledger::ExternalWallet& wallet,
    ledger::SKUOrderCallback callback) {
  if (!order) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Order is null";
    return;
  }

  switch (order->status) {
    case ledger::SKUOrderStatus::PENDING: {
      ContributionIdSaved(
          ledger::Result::LEDGER_OK,
          order->order_id,
          wallet,
          callback);
      return;
    }
    case ledger::SKUOrderStatus::PAID: {
      common_->SendExternalTransaction(order->order_id, callback);
      return;
    }
    case ledger::SKUOrderStatus::FULFILLED: {
      callback(ledger::Result::LEDGER_OK, order->order_id);
      return;
    }
    case ledger::SKUOrderStatus::CANCELED:
    case ledger::SKUOrderStatus::NONE: {
      callback(ledger::Result::LEDGER_ERROR, "");
      return;
    }
  }
}

}  // namespace braveledger_sku
