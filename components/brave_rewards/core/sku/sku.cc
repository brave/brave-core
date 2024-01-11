/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/sku/sku.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace brave_rewards::internal {
namespace sku {

SKU::SKU(RewardsEngineImpl& engine) : engine_(engine), common_(engine) {}

SKU::~SKU() = default;

void SKU::Process(const std::vector<mojom::SKUOrderItem>& items,
                  const std::string& wallet_type,
                  SKUOrderCallback callback,
                  const std::string& contribution_id) {
  auto create_callback = std::bind(&SKU::OrderCreated, this, _1, _2,
                                   wallet_type, contribution_id, callback);

  common_.CreateOrder(items, create_callback);
}

void SKU::OrderCreated(const mojom::Result result,
                       const std::string& order_id,
                       const std::string& wallet_type,
                       const std::string& contribution_id,
                       SKUOrderCallback callback) {
  if (result != mojom::Result::OK) {
    BLOG(0, "Order was not successful");
    callback(result, "");
    return;
  }

  auto save_callback = std::bind(&SKU::ContributionIdSaved, this, _1, order_id,
                                 wallet_type, callback);

  engine_->database()->SaveContributionIdForSKUOrder(order_id, contribution_id,
                                                     save_callback);
}

void SKU::ContributionIdSaved(const mojom::Result result,
                              const std::string& order_id,
                              const std::string& wallet_type,
                              SKUOrderCallback callback) {
  if (result != mojom::Result::OK) {
    BLOG(0, "Contribution id not saved");
    callback(result, "");
    return;
  }

  auto get_callback =
      std::bind(&SKU::CreateTransaction, this, _1, wallet_type, callback);

  engine_->database()->GetSKUOrder(order_id, get_callback);
}

void SKU::CreateTransaction(mojom::SKUOrderPtr order,
                            const std::string& wallet_type,
                            SKUOrderCallback callback) {
  if (!order) {
    BLOG(0, "Order not found");
    callback(mojom::Result::FAILED, "");
    return;
  }

  std::string destination = [&]() -> std::string {
    if (wallet_type == constant::kWalletUphold) {
      return engine_->Get<EnvironmentConfig>().uphold_sku_destination();
    }

    if (wallet_type == constant::kWalletGemini) {
      return engine_->Get<EnvironmentConfig>().gemini_sku_destination();
    }

    NOTREACHED();
    return "";
  }();

  common_.CreateTransaction(std::move(order), destination, wallet_type,
                            callback);
}

void SKU::Retry(const std::string& order_id,
                const std::string& wallet_type,
                SKUOrderCallback callback) {
  if (order_id.empty()) {
    BLOG(0, "Order id is empty");
    callback(mojom::Result::FAILED, "");
    return;
  }

  auto get_callback = std::bind(&SKU::OnOrder, this, _1, wallet_type, callback);

  engine_->database()->GetSKUOrder(order_id, get_callback);
}

void SKU::OnOrder(mojom::SKUOrderPtr order,
                  const std::string& wallet_type,
                  SKUOrderCallback callback) {
  if (!order) {
    BLOG(0, "Order is null");
    callback(mojom::Result::FAILED, "");
    return;
  }

  switch (order->status) {
    case mojom::SKUOrderStatus::PENDING: {
      ContributionIdSaved(mojom::Result::OK, order->order_id, wallet_type,
                          callback);
      return;
    }
    case mojom::SKUOrderStatus::PAID: {
      common_.SendExternalTransaction(order->order_id, callback);
      return;
    }
    case mojom::SKUOrderStatus::FULFILLED: {
      callback(mojom::Result::OK, order->order_id);
      return;
    }
    case mojom::SKUOrderStatus::CANCELED:
    case mojom::SKUOrderStatus::NONE: {
      callback(mojom::Result::FAILED, "");
      return;
    }
  }
}

}  // namespace sku
}  // namespace brave_rewards::internal
