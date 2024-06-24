/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/sku/sku_common.h"

namespace brave_rewards::internal::sku {

SKUCommon::SKUCommon(RewardsEngine& engine)
    : engine_(engine), order_(engine), transaction_(engine) {}

SKUCommon::~SKUCommon() = default;

void SKUCommon::CreateOrder(const std::vector<mojom::SKUOrderItem>& items,
                            SKUOrderCallback callback) {
  order_.Create(items, std::move(callback));
}

void SKUCommon::CreateTransaction(mojom::SKUOrderPtr order,
                                  const std::string& destination,
                                  const std::string& wallet_type,
                                  SKUOrderCallback callback) {
  if (!order) {
    engine_->LogError(FROM_HERE) << "Order not found";
    std::move(callback).Run(mojom::Result::FAILED, "");
    return;
  }

  transaction_.Run(order->Clone(), destination, wallet_type,
                   base::BindOnce(&SKUCommon::OnTransactionCompleted,
                                  weak_factory_.GetWeakPtr(), order->order_id,
                                  std::move(callback)));
}

void SKUCommon::OnTransactionCompleted(const std::string& order_id,
                                       SKUOrderCallback callback,
                                       mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Order status was not updated";
    std::move(callback).Run(result, "");
    return;
  }

  std::move(callback).Run(mojom::Result::OK, order_id);
}

void SKUCommon::SendExternalTransaction(const std::string& order_id,
                                        SKUOrderCallback callback) {
  if (order_id.empty()) {
    engine_->LogError(FROM_HERE) << "Order id is empty";
    std::move(callback).Run(mojom::Result::FAILED, "");
    return;
  }

  engine_->database()->GetSKUTransactionByOrderId(
      order_id,
      base::BindOnce(&SKUCommon::GetSKUTransactionByOrderId,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void SKUCommon::GetSKUTransactionByOrderId(
    SKUOrderCallback callback,
    base::expected<mojom::SKUTransactionPtr, database::GetSKUTransactionError>
        result) {
  const auto transaction = std::move(result).value_or(nullptr);
  if (!transaction) {
    engine_->LogError(FROM_HERE)
        << "Failed to get SKU transaction from database, or there's no "
           "transaction with this order_id";
    return std::move(callback).Run(mojom::Result::FAILED, "");
  }

  transaction_.SendExternalTransaction(
      *transaction,
      base::BindOnce(&SKUCommon::OnTransactionCompleted,
                     weak_factory_.GetWeakPtr(), transaction->order_id,
                     std::move(callback)),
      mojom::Result::OK);
}

}  // namespace brave_rewards::internal::sku
