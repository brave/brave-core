/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/sku/sku_common.h"

using std::placeholders::_1;

namespace brave_rewards::internal {
namespace sku {

SKUCommon::SKUCommon(RewardsEngineImpl& engine)
    : engine_(engine), order_(engine), transaction_(engine) {}

SKUCommon::~SKUCommon() = default;

void SKUCommon::CreateOrder(const std::vector<mojom::SKUOrderItem>& items,
                            SKUOrderCallback callback) {
  order_.Create(items, callback);
}

void SKUCommon::CreateTransaction(mojom::SKUOrderPtr order,
                                  const std::string& destination,
                                  const std::string& wallet_type,
                                  SKUOrderCallback callback) {
  if (!order) {
    engine_->LogError(FROM_HERE) << "Order not found";
    callback(mojom::Result::FAILED, "");
    return;
  }

  auto create_callback = std::bind(&SKUCommon::OnTransactionCompleted, this, _1,
                                   order->order_id, callback);

  transaction_.Run(order->Clone(), destination, wallet_type, create_callback);
}

void SKUCommon::OnTransactionCompleted(const mojom::Result result,
                                       const std::string& order_id,
                                       SKUOrderCallback callback) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Order status was not updated";
    callback(result, "");
    return;
  }

  callback(mojom::Result::OK, order_id);
}

void SKUCommon::SendExternalTransaction(const std::string& order_id,
                                        SKUOrderCallback callback) {
  if (order_id.empty()) {
    engine_->LogError(FROM_HERE) << "Order id is empty";
    callback(mojom::Result::FAILED, "");
    return;
  }

  auto get_callback =
      std::bind(&SKUCommon::GetSKUTransactionByOrderId, this, _1, callback);

  engine_->database()->GetSKUTransactionByOrderId(order_id, get_callback);
}

void SKUCommon::GetSKUTransactionByOrderId(
    base::expected<mojom::SKUTransactionPtr, database::GetSKUTransactionError>
        result,
    SKUOrderCallback callback) {
  const auto transaction = std::move(result).value_or(nullptr);
  if (!transaction) {
    engine_->LogError(FROM_HERE)
        << "Failed to get SKU transaction from database, or there's no "
           "transaction with this order_id";
    return callback(mojom::Result::FAILED, "");
  }

  transaction_.SendExternalTransaction(
      mojom::Result::OK, *transaction,
      std::bind(&SKUCommon::OnTransactionCompleted, this, _1,
                transaction->order_id, std::move(callback)));
}

}  // namespace sku
}  // namespace brave_rewards::internal
