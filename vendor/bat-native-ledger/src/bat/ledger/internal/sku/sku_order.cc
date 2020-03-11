/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/request/request_sku.h"
#include "bat/ledger/internal/sku/sku_order.h"
#include "bat/ledger/internal/sku/sku_util.h"
#include "net/http/http_status_code.h"

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
    ledger::SKUOrderCallback callback,
    const std::string& contribution_id) {
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
      _2,
      _3,
      items,
      contribution_id,
      callback);

  const std::string url = braveledger_request_util::GetCreateOrderURL();

  ledger_->LoadURL(
      url,
      std::vector<std::string>(),
      json,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void SKUOrder::OnCreate(
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const std::vector<ledger::SKUOrderItem>& order_items,
    const std::string& contribution_id,
    ledger::SKUOrderCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  if (response_status_code != net::HTTP_CREATED) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  auto order = ParseOrderCreateResponse(response, order_items);

  if (!order) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
        << "Order response could not be parsed";
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  order->contribution_id = contribution_id;

  auto save_callback = std::bind(&SKUOrder::OnCreateSave,
      this,
      _1,
      order->order_id,
      callback);

  ledger_->SaveSKUOrder(order->Clone(), save_callback);
}

void SKUOrder::OnCreateSave(
    const ledger::Result result,
    const std::string& order_id,
    ledger::SKUOrderCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
        << "Order couldn't be saved";
    callback(result, "");
    return;
  }

  callback(ledger::Result::LEDGER_OK, order_id);
}

}  // namespace braveledger_sku
