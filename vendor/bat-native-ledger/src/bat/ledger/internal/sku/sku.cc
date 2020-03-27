/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/common/bind_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/sku/sku.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace braveledger_sku {

SKU::SKU(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger),
    order_(std::make_unique<SKUOrder>(ledger)),
    transaction_(std::make_unique<SKUTransaction>(ledger)) {
  DCHECK(ledger_);
}

SKU::~SKU() = default;

void SKU::Brave(
    const std::string& destination,
    const std::vector<ledger::SKUOrderItem>& items,
    const std::string& contribution_id,
    ledger::ExternalWalletPtr wallet,
    ledger::SKUOrderCallback callback) {
  auto create_callback = std::bind(&SKU::GetOrder,
      this,
      _1,
      _2,
      destination,
      *wallet,
      callback);

  order_->Create(items, create_callback, contribution_id);
}

void SKU::Process(
    const std::vector<ledger::SKUOrderItem>& items,
    ledger::ExternalWalletPtr wallet,
    ledger::SKUOrderCallback callback) {
  auto create_callback = std::bind(&SKU::GetOrder,
      this,
      _1,
      _2,
      "",
      *wallet,
      callback);

  order_->Create(items, create_callback, "");
}

void SKU::GetOrder(
    const ledger::Result result,
    const std::string& order_id,
    const std::string& destination,
    const ledger::ExternalWallet& wallet,
    ledger::SKUOrderCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Order was not successful";
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  auto get_callback = std::bind(&SKU::CreateTransaction,
      this,
      _1,
      destination,
      wallet,
      callback);

  ledger_->GetSKUOrder(order_id, get_callback);
}

void SKU::CreateTransaction(
    ledger::SKUOrderPtr order,
    const std::string& destination,
    const ledger::ExternalWallet& wallet,
    ledger::SKUOrderCallback callback) {
  if (!order) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Order not found";
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  if (destination.empty() && wallet.type == ledger::kWalletUphold) {
    auto publisher_callback =
        std::bind(&SKU::OnServerPublisherInfo,
          this,
          _1,
          braveledger_bind_util::FromSKUOrderToString(std::move(order)),
          wallet,
          callback);

    ledger_->GetServerPublisherInfo(order->merchant_id, publisher_callback);
    return;
  }

  auto create_callback = std::bind(&SKU::OnTransactionCompleted,
      this,
      _1,
      order->order_id,
      callback);

  transaction_->Create(
      std::move(order),
      destination,
      wallet,
      create_callback);
}

void SKU::OnServerPublisherInfo(
    ledger::ServerPublisherInfoPtr info,
    const std::string& order_string,
    const ledger::ExternalWallet& wallet,
    ledger::SKUOrderCallback callback) {
  auto order = braveledger_bind_util::FromStringToSKUOrder(order_string);
  if (!order || !info) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Order/Publisher not found";
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  if (info->address.empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Publisher address is empty";
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  CreateTransaction(std::move(order), info->address, wallet, callback);
}

void SKU::OnTransactionCompleted(
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

}  // namespace braveledger_sku
