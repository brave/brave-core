/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/json/json_reader.h"
#include "base/values.h"
#include "bat/ledger/internal/sku/sku_util.h"

namespace braveledger_sku {

ledger::SKUOrderPtr ParseOrderCreateResponse(
    const std::string& response,
    const std::vector<ledger::SKUOrderItem>& order_items) {
  base::Optional<base::Value> dictionary = base::JSONReader::Read(response);
  if (!dictionary || !dictionary->is_dict()) {
    return nullptr;
  }

  auto order = ledger::SKUOrder::New();

  const auto* id = dictionary->FindStringKey("id");
  if (id) {
    order->order_id = *id;
  }

  if (order->order_id.empty()) {
    return nullptr;
  }

  const auto* total_amount = dictionary->FindStringKey("totalPrice");
  if (total_amount) {
    order->total_amount = std::stod(*total_amount);
  }

  const auto* merchant_id = dictionary->FindStringKey("merchantId");
  if (merchant_id) {
    order->merchant_id = *merchant_id;
  }

  const auto* location = dictionary->FindStringKey("location");
  if (location) {
    order->location = *location;
  }

  order->status = ledger::SKUOrderStatus::PENDING;

  auto* items = dictionary->FindListKey("items");
  if (items) {
    if (items->GetList().size() != order_items.size()) {
      return nullptr;
    }

    int count = 0;
    for (auto& item : items->GetList()) {
      auto order_item = ledger::SKUOrderItem::New();
      order_item->order_id = order->order_id;
      order_item->sku = order_items[count].sku;
      order_item->type = order_items[count].type;

      const auto* id = item.FindStringKey("id");
      if (id) {
        order_item->order_item_id = *id;
      }

      const auto quantity = item.FindIntKey("quantity");
      if (quantity) {
        order_item->quantity = *quantity;
      }

      const auto* price = item.FindStringKey("price");
      if (price) {
        order_item->price = std::stod(*price);
      }

      const auto* name = item.FindStringKey("name");
      if (name) {
        order_item->name = *name;
      }

      const auto* description = item.FindStringKey("description");
      if (description) {
        order_item->description = *description;
      }

      order->items.push_back(std::move(order_item));

      count++;
    }
  }

  return order;
}

std::string ConvertTransactionTypeToString(
    const ledger::SKUTransactionType type) {
  switch (type) {
    case ledger::SKUTransactionType::UPHOLD: {
      return "uphold";
    }
    case ledger::SKUTransactionType::ANONYMOUS_CARD: {
      return "anonymous-card";
    }
    case ledger::SKUTransactionType::NONE:
    case ledger::SKUTransactionType::TOKENS: {
      return "";
    }
  }
}

}  // namespace braveledger_sku
