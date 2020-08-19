/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/response/response_sku.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "bat/ledger/internal/logging/logging.h"
#include "net/http/http_status_code.h"

namespace braveledger_response_util {

// Request Url:
// POST /v1/orders/{order_id}/transactions/{transaction_suffix}
//
// Success:
// Created (201)
//
// Response Format:
// {
//   "id": "80740e9c-08c3-43ed-92aa-2a7be8352000",
//   "orderId": "f2e6494e-fb21-44d1-90e9-b5408799acd8",
//   "createdAt": "2020-06-10T18:58:22.817675Z",
//   "updatedAt": "2020-06-10T18:58:22.817675Z",
//   "external_transaction_id": "d382d3ae-8462-4b2c-9b60-b669539f41b2",
//   "status": "completed",
//   "currency": "BAT",
//   "kind": "uphold",
//   "amount": "1"
// }

ledger::Result CheckSendExternalTransaction(
    const ledger::UrlResponse& response) {
  // Bad Request (400)
  if (response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return ledger::Result::LEDGER_ERROR;
  }

  // Not Found (404)
  if (response.status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized transaction suffix");
    return ledger::Result::NOT_FOUND;
  }

  // Conflict (409)
  if (response.status_code == net::HTTP_CONFLICT) {
    BLOG(0, "External transaction id already submitted");
    return ledger::Result::LEDGER_ERROR;
  }

  // Internal Server Error (500)
  if (response.status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return ledger::Result::LEDGER_ERROR;
  }

  // Created (201)
  if (response.status_code != net::HTTP_CREATED) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

// Request Url:
// POST /v1/orders
//
// Success:
// Created (201)
//
// Response Format:
// {
//   "id": "f2e6494e-fb21-44d1-90e9-b5408799acd8",
//   "createdAt": "2020-06-10T18:58:21.378752Z",
//   "currency": "BAT",
//   "updatedAt": "2020-06-10T18:58:21.378752Z",
//   "totalPrice": "1",
//   "location": "brave.com",
//   "status": "pending",
//   "items": [
//     {
//       "id": "9c9aed7f-b349-452e-80a8-95faf2b1600d",
//       "orderId": "f2e6494e-fb21-44d1-90e9-b5408799acd8",
//       "sku": "user-wallet-vote",
//       "createdAt": "2020-06-10T18:58:21.378752Z",
//       "updatedAt": "2020-06-10T18:58:21.378752Z",
//       "currency": "BAT",
//       "quantity": 4,
//       "price": "0.25",
//       "subtotal": "1",
//       "location": "brave.com",
//       "description": ""
//     }
//   ]
// }

ledger::SKUOrderPtr ParseOrderCreate(
    const ledger::UrlResponse& response,
    const std::vector<ledger::SKUOrderItem>& order_items) {
  // Bad Request (400)
  if (response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return nullptr;
  }

  // Internal Server Error (500)
  if (response.status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return nullptr;
  }

  // Created (201)
  if (response.status_code != net::HTTP_CREATED) {
    return nullptr;
  }

  base::Optional<base::Value> dictionary =
      base::JSONReader::Read(response.body);
  if (!dictionary || !dictionary->is_dict()) {
    BLOG(0, "Invalid JSON");
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
    const bool success =
        base::StringToDouble(*total_amount, &order->total_amount);
    if (!success) {
      order->total_amount = 0.0;
    }
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
  if (!items) {
    return order;
  }

  if (items->GetList().size() != order_items.size()) {
    BLOG(0, "Invalid JSON");
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
      const bool success = base::StringToDouble(*price, &order_item->price);
      if (!success) {
        order_item->price = 0.0;
      }
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

  return order;
}

// Request Url:
// POST /v1/orders/{order_id}/credentials
// POST /v1/orders/{order_id}/credentials/{item_id}
//
// Success:
// OK (200)
//
// Response Format:
// {Empty body}

ledger::Result CheckClaimSKUCreds(const ledger::UrlResponse& response) {
  // Bad Request (400)
  if (response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return ledger::Result::LEDGER_ERROR;
  }

  // Conflict (409)
  if (response.status_code == net::HTTP_CONFLICT) {
    BLOG(0, "Credentials already exist for this order");
    return ledger::Result::LEDGER_ERROR;
  }

  // Internal Server Error (500)
  if (response.status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return ledger::Result::LEDGER_ERROR;
  }

  if (response.status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

// Request Url:
// POST /v1/votes
//
// Success:
// OK (200)
//
// Response Format:
// {Empty body}

ledger::Result CheckRedeemSKUTokens(const ledger::UrlResponse& response) {
  // Bad Request (400)
  if (response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return ledger::Result::LEDGER_ERROR;
  }

  // Internal Server Error (500)
  if (response.status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return ledger::Result::LEDGER_ERROR;
  }

  if (response.status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

}  // namespace braveledger_response_util
