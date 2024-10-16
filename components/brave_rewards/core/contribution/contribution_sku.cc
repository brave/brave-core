/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/constants.h"
#include "brave/components/brave_rewards/core/contribution/contribution.h"
#include "brave/components/brave_rewards/core/contribution/contribution_sku.h"
#include "brave/components/brave_rewards/core/contribution/contribution_util.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"

namespace brave_rewards::internal {

namespace {

void GetCredentialTrigger(mojom::SKUOrderPtr order,
                          credential::CredentialsTrigger* trigger) {
  DCHECK(trigger);

  if (!order || order->items.size() != 1) {
    return;
  }

  std::vector<std::string> data;
  data.push_back(order->items[0]->order_item_id);
  data.push_back(std::to_string(static_cast<int>(order->items[0]->type)));

  trigger->id = order->order_id;
  trigger->size = order->items[0]->quantity;
  trigger->type = mojom::CredsBatchType::SKU;
  trigger->data = data;
}

}  // namespace

namespace contribution {

ContributionSKU::ContributionSKU(RewardsEngine& engine)
    : engine_(engine), credentials_(engine), sku_(engine) {}

ContributionSKU::~ContributionSKU() = default;

void ContributionSKU::AutoContribution(const std::string& contribution_id,
                                       const std::string& wallet_type,
                                       ResultCallback callback) {
  mojom::SKUOrderItem item;
  item.sku = engine_->Get<EnvironmentConfig>().auto_contribute_sku();

  Start(contribution_id, item, wallet_type, std::move(callback));
}

void ContributionSKU::Start(const std::string& contribution_id,
                            const mojom::SKUOrderItem& item,
                            const std::string& wallet_type,
                            ResultCallback callback) {
  engine_->database()->GetContributionInfo(
      contribution_id, base::BindOnce(&ContributionSKU::GetContributionInfo,
                                      weak_factory_.GetWeakPtr(), item,
                                      wallet_type, std::move(callback)));
}

void ContributionSKU::GetContributionInfo(
    const mojom::SKUOrderItem& item,
    const std::string& wallet_type,
    ResultCallback callback,
    mojom::ContributionInfoPtr contribution) {
  if (!contribution) {
    engine_->LogError(FROM_HERE) << "Contribution not found";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  ResultCallback complete_callback = base::BindOnce(
      &ContributionSKU::Completed, weak_factory_.GetWeakPtr(),
      contribution->contribution_id, contribution->type, std::move(callback));

  auto process_callback = base::BindOnce(
      &ContributionSKU::GetOrder, weak_factory_.GetWeakPtr(),
      contribution->contribution_id, std::move(complete_callback));

  mojom::SKUOrderItem new_item = item;
  new_item.quantity = GetVotesFromAmount(contribution->amount);
  new_item.type = mojom::SKUOrderItemType::SINGLE_USE;
  new_item.price = constant::kVotePrice;

  std::vector<mojom::SKUOrderItem> items;
  items.push_back(new_item);

  sku_.Process(items, wallet_type, std::move(process_callback),
               contribution->contribution_id);
}

void ContributionSKU::GetOrder(const std::string& contribution_id,
                               ResultCallback callback,
                               mojom::Result result,
                               const std::string& order_id) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "SKU was not processed";
    std::move(callback).Run(result);
    return;
  }

  engine_->database()->GetSKUOrder(
      order_id,
      base::BindOnce(&ContributionSKU::OnGetOrder, weak_factory_.GetWeakPtr(),
                     contribution_id, std::move(callback)));
}

void ContributionSKU::OnGetOrder(const std::string& contribution_id,
                                 ResultCallback callback,
                                 mojom::SKUOrderPtr order) {
  if (!order) {
    engine_->LogError(FROM_HERE) << "Order was not found";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  DCHECK_EQ(order->items.size(), 1ul);
  credential::CredentialsTrigger trigger;
  GetCredentialTrigger(order->Clone(), &trigger);

  credentials_.Start(trigger, std::move(callback));
}

void ContributionSKU::Completed(const std::string& contribution_id,
                                mojom::RewardsType type,
                                ResultCallback callback,
                                mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Order not completed";
    std::move(callback).Run(result);
    return;
  }

  engine_->database()->UpdateContributionInfoStep(
      contribution_id, mojom::ContributionStep::STEP_CREDS,
      base::BindOnce(&ContributionSKU::CredsStepSaved,
                     weak_factory_.GetWeakPtr(), contribution_id,
                     std::move(callback)));
}

void ContributionSKU::CredsStepSaved(const std::string& contribution_id,
                                     ResultCallback callback,
                                     mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Creds step not saved";
    std::move(callback).Run(result);
    return;
  }

  engine_->contribution()->StartUnblinded({mojom::CredsBatchType::SKU},
                                          contribution_id, std::move(callback));
}

void ContributionSKU::Retry(mojom::ContributionInfoPtr contribution,
                            ResultCallback callback) {
  if (!contribution) {
    engine_->LogError(FROM_HERE) << "Contribution was not found";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  std::string contribution_id = contribution->contribution_id;

  engine_->database()->GetSKUOrderByContributionId(
      contribution_id,
      base::BindOnce(&ContributionSKU::OnOrder, weak_factory_.GetWeakPtr(),
                     std::move(contribution), std::move(callback)));
}

void ContributionSKU::OnOrder(mojom::ContributionInfoPtr contribution,
                              ResultCallback callback,
                              mojom::SKUOrderPtr order) {
  if (!contribution) {
    engine_->LogError(FROM_HERE) << "Contribution is null";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  switch (contribution->step) {
    case mojom::ContributionStep::STEP_START:
    case mojom::ContributionStep::STEP_EXTERNAL_TRANSACTION: {
      RetryStartStep(std::move(contribution), std::move(order),
                     std::move(callback));
      return;
    }
    case mojom::ContributionStep::STEP_PREPARE:
    case mojom::ContributionStep::STEP_RESERVE:
    case mojom::ContributionStep::STEP_CREDS: {
      engine_->contribution()->RetryUnblinded({mojom::CredsBatchType::SKU},
                                              contribution->contribution_id,
                                              std::move(callback));
      return;
    }
    case mojom::ContributionStep::STEP_RETRY_COUNT:
    case mojom::ContributionStep::STEP_REWARDS_OFF:
    case mojom::ContributionStep::STEP_AC_OFF:
    case mojom::ContributionStep::STEP_AC_TABLE_EMPTY:
    case mojom::ContributionStep::STEP_NOT_ENOUGH_FUNDS:
    case mojom::ContributionStep::STEP_FAILED:
    case mojom::ContributionStep::STEP_COMPLETED:
    case mojom::ContributionStep::STEP_NO: {
      engine_->LogError(FROM_HERE) << "Step not correct " << contribution->step;
      return;
    }
  }
}

void ContributionSKU::RetryStartStep(mojom::ContributionInfoPtr contribution,
                                     mojom::SKUOrderPtr order,
                                     ResultCallback callback) {
  if (!contribution) {
    engine_->LogError(FROM_HERE) << "Contribution is null";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  std::string wallet_type;
  switch (contribution->processor) {
    case mojom::ContributionProcessor::UPHOLD:
      wallet_type = constant::kWalletUphold;
      break;
    case mojom::ContributionProcessor::GEMINI:
      wallet_type = constant::kWalletGemini;
      break;
    case mojom::ContributionProcessor::NONE:
    case mojom::ContributionProcessor::BRAVE_TOKENS:
    case mojom::ContributionProcessor::BITFLYER:
      break;
  }

  if (wallet_type.empty()) {
    engine_->LogError(FROM_HERE) << "Invalid processor for SKU contribution";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  // If an SKU order has not been created yet, then start the SKU order process
  // from the beginning.
  if (!order || order->order_id.empty()) {
    DCHECK(wallet_type == constant::kWalletUphold ||
           wallet_type == constant::kWalletGemini);
    AutoContribution(contribution->contribution_id, wallet_type,
                     std::move(callback));
    return;
  }

  ResultCallback complete_callback = base::BindOnce(
      &ContributionSKU::Completed, weak_factory_.GetWeakPtr(),
      contribution->contribution_id, contribution->type, std::move(callback));

  auto retry_callback = base::BindOnce(
      &ContributionSKU::GetOrder, weak_factory_.GetWeakPtr(),
      contribution->contribution_id, std::move(complete_callback));

  sku_.Retry(order->order_id, wallet_type, std::move(retry_callback));
}

}  // namespace contribution
}  // namespace brave_rewards::internal
