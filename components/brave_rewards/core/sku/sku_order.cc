/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/sku/sku_order.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace brave_rewards::internal {
namespace sku {

SKUOrder::SKUOrder(RewardsEngineImpl& engine)
    : engine_(engine), payment_server_(engine) {}

SKUOrder::~SKUOrder() = default;

void SKUOrder::Create(const std::vector<mojom::SKUOrderItem>& items,
                      SKUOrderCallback callback) {
  if (items.empty()) {
    engine_->LogError(FROM_HERE) << "List is empty";
    callback(mojom::Result::FAILED, "");
    return;
  }

  auto url_callback = std::bind(&SKUOrder::OnCreate, this, _1, _2, callback);

  payment_server_.post_order().Request(items, url_callback);
}

void SKUOrder::OnCreate(const mojom::Result result,
                        mojom::SKUOrderPtr order,
                        SKUOrderCallback callback) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Order response could not be parsed";
    callback(mojom::Result::FAILED, "");
    return;
  }

  auto save_callback =
      std::bind(&SKUOrder::OnCreateSave, this, _1, order->order_id, callback);

  engine_->database()->SaveSKUOrder(order->Clone(), save_callback);
}

void SKUOrder::OnCreateSave(const mojom::Result result,
                            const std::string& order_id,
                            SKUOrderCallback callback) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Order couldn't be saved";
    callback(result, "");
    return;
  }

  callback(mojom::Result::OK, order_id);
}

}  // namespace sku
}  // namespace brave_rewards::internal
