/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/sku/sku.h"

namespace brave_rewards::internal::sku {

SKU::SKU(RewardsEngine& engine) : engine_(engine), common_(engine) {}

SKU::~SKU() = default;

void SKU::Process(const std::vector<mojom::SKUOrderItem>& items,
                  const std::string& wallet_type,
                  SKUOrderCallback callback,
                  const std::string& contribution_id) {
  common_.CreateOrder(
      items, base::BindOnce(&SKU::OrderCreated, weak_factory_.GetWeakPtr(),
                            wallet_type, contribution_id, std::move(callback)));
}

void SKU::OrderCreated(const std::string& wallet_type,
                       const std::string& contribution_id,
                       SKUOrderCallback callback,
                       mojom::Result result,
                       const std::string& order_id) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Order was not successful";
    std::move(callback).Run(result, "");
    return;
  }

  engine_->database()->SaveContributionIdForSKUOrder(
      order_id, contribution_id,
      base::BindOnce(&SKU::ContributionIdSaved, weak_factory_.GetWeakPtr(),
                     order_id, wallet_type, std::move(callback)));
}

void SKU::ContributionIdSaved(const std::string& order_id,
                              const std::string& wallet_type,
                              SKUOrderCallback callback,
                              mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Contribution id not saved";
    std::move(callback).Run(result, "");
    return;
  }

  engine_->database()->GetSKUOrder(
      order_id,
      base::BindOnce(&SKU::CreateTransaction, weak_factory_.GetWeakPtr(),
                     wallet_type, std::move(callback)));
}

void SKU::CreateTransaction(const std::string& wallet_type,
                            SKUOrderCallback callback,
                            mojom::SKUOrderPtr order) {
  if (!order) {
    engine_->LogError(FROM_HERE) << "Order not found";
    std::move(callback).Run(mojom::Result::FAILED, "");
    return;
  }

  std::string destination = [&]() -> std::string {
    if (wallet_type == constant::kWalletUphold) {
      return engine_->Get<EnvironmentConfig>().uphold_sku_destination();
    }

    if (wallet_type == constant::kWalletGemini) {
      return engine_->Get<EnvironmentConfig>().gemini_sku_destination();
    }

    return "";
  }();

  common_.CreateTransaction(std::move(order), destination, wallet_type,
                            std::move(callback));
}

void SKU::Retry(const std::string& order_id,
                const std::string& wallet_type,
                SKUOrderCallback callback) {
  if (order_id.empty()) {
    engine_->LogError(FROM_HERE) << "Order id is empty";
    std::move(callback).Run(mojom::Result::FAILED, "");
    return;
  }

  engine_->database()->GetSKUOrder(
      order_id, base::BindOnce(&SKU::OnOrder, weak_factory_.GetWeakPtr(),
                               wallet_type, std::move(callback)));
}

void SKU::OnOrder(const std::string& wallet_type,
                  SKUOrderCallback callback,
                  mojom::SKUOrderPtr order) {
  if (!order) {
    engine_->LogError(FROM_HERE) << "Order is null";
    std::move(callback).Run(mojom::Result::FAILED, "");
    return;
  }

  switch (order->status) {
    case mojom::SKUOrderStatus::PENDING: {
      ContributionIdSaved(order->order_id, wallet_type, std::move(callback),
                          mojom::Result::OK);
      return;
    }
    case mojom::SKUOrderStatus::PAID: {
      common_.SendExternalTransaction(order->order_id, std::move(callback));
      return;
    }
    case mojom::SKUOrderStatus::FULFILLED: {
      std::move(callback).Run(mojom::Result::OK, order->order_id);
      return;
    }
    case mojom::SKUOrderStatus::CANCELED:
    case mojom::SKUOrderStatus::NONE: {
      std::move(callback).Run(mojom::Result::FAILED, "");
      return;
    }
  }
}

}  // namespace brave_rewards::internal::sku
