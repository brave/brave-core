/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/payment/post_order/post_order.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {
namespace endpoint {
namespace payment {

PostOrder::PostOrder(RewardsEngineImpl& engine) : engine_(engine) {}

PostOrder::~PostOrder() = default;

std::string PostOrder::GetUrl() {
  return engine_->Get<EnvironmentConfig>()
      .rewards_payment_url()
      .Resolve("/v1/orders")
      .spec();
}

std::string PostOrder::GeneratePayload(
    const std::vector<mojom::SKUOrderItem>& items) {
  base::Value::List order_items;
  for (const auto& item : items) {
    base::Value::Dict order_item;
    order_item.Set("sku", item.sku);
    order_item.Set("quantity", item.quantity);
    order_items.Append(std::move(order_item));
  }

  base::Value::Dict body;
  body.Set("items", std::move(order_items));

  std::string json;
  base::JSONWriter::Write(body, &json);

  return json;
}

mojom::Result PostOrder::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return mojom::Result::RETRY_SHORT;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return mojom::Result::RETRY_SHORT;
  }

  if (status_code != net::HTTP_CREATED) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

mojom::Result PostOrder::ParseBody(
    const std::string& body,
    const std::vector<mojom::SKUOrderItem>& order_items,
    mojom::SKUOrder* order) {
  DCHECK(order);

  std::optional<base::Value> dictionary = base::JSONReader::Read(body);
  if (!dictionary || !dictionary->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::FAILED;
  }

  const base::Value::Dict& dict = dictionary->GetDict();

  const auto* id = dict.FindString("id");
  if (id) {
    order->order_id = *id;
  }

  if (order->order_id.empty()) {
    BLOG(0, "Order id empty");
    return mojom::Result::FAILED;
  }

  const auto* total_amount = dict.FindString("totalPrice");
  if (total_amount) {
    const bool success =
        base::StringToDouble(*total_amount, &order->total_amount);
    if (!success) {
      order->total_amount = 0.0;
    }
  }

  const auto* merchant_id = dict.FindString("merchantId");
  if (merchant_id) {
    order->merchant_id = *merchant_id;
  }

  const auto* location = dict.FindString("location");
  if (location) {
    order->location = *location;
  }

  order->status = mojom::SKUOrderStatus::PENDING;

  auto* items = dict.FindList("items");
  if (!items) {
    return mojom::Result::OK;
  }

  if (items->size() != order_items.size()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::FAILED;
  }

  int count = 0;
  for (auto& value : *items) {
    const base::Value::Dict& item = value.GetDict();
    auto order_item = mojom::SKUOrderItem::New();
    order_item->order_id = order->order_id;
    order_item->sku = order_items[count].sku;
    order_item->type = order_items[count].type;

    const auto* order_item_id = item.FindString("id");
    if (order_item_id) {
      order_item->order_item_id = *order_item_id;
    }

    const auto quantity = item.FindInt("quantity");
    if (quantity) {
      order_item->quantity = *quantity;
    }

    const auto* price = item.FindString("price");
    if (price) {
      const bool success = base::StringToDouble(*price, &order_item->price);
      if (!success) {
        order_item->price = 0.0;
      }
    }

    const auto* name = item.FindString("name");
    if (name) {
      order_item->name = *name;
    }

    const auto* description = item.FindString("description");
    if (description) {
      order_item->description = *description;
    }

    order->items.push_back(std::move(order_item));

    count++;
  }

  return mojom::Result::OK;
}

void PostOrder::Request(const std::vector<mojom::SKUOrderItem>& items,
                        PostOrderCallback callback) {
  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  request->content = GeneratePayload(items);
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::POST;

  engine_->Get<URLLoader>().Load(
      std::move(request), URLLoader::LogLevel::kDetailed,
      base::BindOnce(&PostOrder::OnRequest, base::Unretained(this),
                     std::move(items), std::move(callback)));
}

void PostOrder::OnRequest(std::vector<mojom::SKUOrderItem> items,
                          PostOrderCallback callback,
                          mojom::UrlResponsePtr response) {
  DCHECK(response);

  mojom::Result result = CheckStatusCode(response->status_code);

  if (result != mojom::Result::OK) {
    callback(result, nullptr);
    return;
  }

  auto order = mojom::SKUOrder::New();
  result = ParseBody(response->body, items, order.get());
  callback(result, std::move(order));
}

}  // namespace payment
}  // namespace endpoint
}  // namespace brave_rewards::internal
