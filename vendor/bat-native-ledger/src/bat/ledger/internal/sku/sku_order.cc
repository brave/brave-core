/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/request/request_sku.h"
#include "bat/ledger/internal/response/response_sku.h"
#include "bat/ledger/internal/sku/sku_order.h"
#include "bat/ledger/internal/sku/sku_util.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_sku {

SKUOrder::SKUOrder(bat_ledger::LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

SKUOrder::~SKUOrder() = default;

void SKUOrder::Create(
    const std::vector<ledger::SKUOrderItem>& items,
    ledger::SKUOrderCallback callback) {
  if (items.empty()) {
    BLOG(0, "List is empty");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  base::Value order_items(base::Value::Type::LIST);
  for (const auto& item : items) {
    base::Value order_item(base::Value::Type::DICTIONARY);
    order_item.SetStringKey("sku", item.sku);
    order_item.SetIntKey("quantity", item.quantity);
    order_items.Append(std::move(order_item));
  }

  base::Value body(base::Value::Type::DICTIONARY);
  body.SetKey("items", std::move(order_items));

  std::string json;
  base::JSONWriter::Write(body, &json);

  auto url_callback = std::bind(&SKUOrder::OnCreate,
      this,
      _1,
      items,
      callback);

  const std::string url = braveledger_request_util::GetCreateOrderURL();

  ledger_->LoadURL(
      url,
      {},
      json,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void SKUOrder::OnCreate(
    const ledger::UrlResponse& response,
    const std::vector<ledger::SKUOrderItem>& order_items,
    ledger::SKUOrderCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  auto order =
      braveledger_response_util::ParseOrderCreate(response, order_items);

  if (!order) {
    BLOG(0, "Order response could not be parsed");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  auto save_callback = std::bind(&SKUOrder::OnCreateSave,
      this,
      _1,
      order->order_id,
      callback);

  ledger_->database()->SaveSKUOrder(order->Clone(), save_callback);
}

void SKUOrder::OnCreateSave(
    const ledger::Result result,
    const std::string& order_id,
    ledger::SKUOrderCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Order couldn't be saved");
    callback(result, "");
    return;
  }

  callback(ledger::Result::LEDGER_OK, order_id);
}

}  // namespace braveledger_sku
