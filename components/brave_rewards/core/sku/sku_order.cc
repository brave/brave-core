/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/sku/sku_order.h"

namespace brave_rewards::internal::sku {

SKUOrder::SKUOrder(RewardsEngine& engine)
    : engine_(engine), payment_server_(engine) {}

SKUOrder::~SKUOrder() = default;

void SKUOrder::Create(const std::vector<mojom::SKUOrderItem>& items,
                      SKUOrderCallback callback) {
  if (items.empty()) {
    engine_->LogError(FROM_HERE) << "List is empty";
    std::move(callback).Run(mojom::Result::FAILED, "");
    return;
  }

  payment_server_.post_order().Request(
      items, base::BindOnce(&SKUOrder::OnCreate, weak_factory_.GetWeakPtr(),
                            std::move(callback)));
}

void SKUOrder::OnCreate(SKUOrderCallback callback,
                        mojom::Result result,
                        mojom::SKUOrderPtr order) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Order response could not be parsed";
    std::move(callback).Run(mojom::Result::FAILED, "");
    return;
  }

  engine_->database()->SaveSKUOrder(
      order->Clone(),
      base::BindOnce(&SKUOrder::OnCreateSave, weak_factory_.GetWeakPtr(),
                     order->order_id, std::move(callback)));
}

void SKUOrder::OnCreateSave(const std::string& order_id,
                            SKUOrderCallback callback,
                            mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Order couldn't be saved";
    std::move(callback).Run(result, "");
    return;
  }

  std::move(callback).Run(mojom::Result::OK, order_id);
}

}  // namespace brave_rewards::internal::sku
