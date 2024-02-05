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
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

using std::placeholders::_1;
using std::placeholders::_2;

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

ContributionSKU::ContributionSKU(RewardsEngineImpl& engine)
    : engine_(engine), credentials_(engine), sku_(engine) {}

ContributionSKU::~ContributionSKU() = default;

void ContributionSKU::AutoContribution(const std::string& contribution_id,
                                       const std::string& wallet_type,
                                       LegacyResultCallback callback) {
  mojom::SKUOrderItem item;
  item.sku = engine_->Get<EnvironmentConfig>().auto_contribute_sku();

  Start(contribution_id, item, wallet_type, callback);
}

void ContributionSKU::Start(const std::string& contribution_id,
                            const mojom::SKUOrderItem& item,
                            const std::string& wallet_type,
                            LegacyResultCallback callback) {
  auto get_callback = std::bind(&ContributionSKU::GetContributionInfo, this, _1,
                                item, wallet_type, callback);

  engine_->database()->GetContributionInfo(contribution_id, get_callback);
}

void ContributionSKU::GetContributionInfo(
    mojom::ContributionInfoPtr contribution,
    const mojom::SKUOrderItem& item,
    const std::string& wallet_type,
    LegacyResultCallback callback) {
  if (!contribution) {
    BLOG(0, "Contribution not found");
    callback(mojom::Result::FAILED);
    return;
  }

  LegacyResultCallback complete_callback =
      std::bind(&ContributionSKU::Completed, this, _1,
                contribution->contribution_id, contribution->type, callback);

  auto process_callback =
      std::bind(&ContributionSKU::GetOrder, this, _1, _2,
                contribution->contribution_id, complete_callback);

  mojom::SKUOrderItem new_item = item;
  new_item.quantity = GetVotesFromAmount(contribution->amount);
  new_item.type = mojom::SKUOrderItemType::SINGLE_USE;
  new_item.price = constant::kVotePrice;

  std::vector<mojom::SKUOrderItem> items;
  items.push_back(new_item);

  sku_.Process(items, wallet_type, process_callback,
               contribution->contribution_id);
}

void ContributionSKU::GetOrder(mojom::Result result,
                               const std::string& order_id,
                               const std::string& contribution_id,
                               LegacyResultCallback callback) {
  if (result != mojom::Result::OK) {
    BLOG(0, "SKU was not processed");
    callback(result);
    return;
  }

  auto get_callback = std::bind(&ContributionSKU::OnGetOrder, this, _1,
                                contribution_id, callback);
  engine_->database()->GetSKUOrder(order_id, get_callback);
}

void ContributionSKU::OnGetOrder(mojom::SKUOrderPtr order,
                                 const std::string& contribution_id,
                                 LegacyResultCallback callback) {
  if (!order) {
    BLOG(0, "Order was not found");
    callback(mojom::Result::FAILED);
    return;
  }

  DCHECK_EQ(order->items.size(), 1ul);
  credential::CredentialsTrigger trigger;
  GetCredentialTrigger(order->Clone(), &trigger);

  credentials_.Start(
      trigger, base::BindOnce([](LegacyResultCallback callback,
                                 mojom::Result result) { callback(result); },
                              std::move(callback)));
}

void ContributionSKU::Completed(mojom::Result result,
                                const std::string& contribution_id,
                                mojom::RewardsType type,
                                LegacyResultCallback callback) {
  if (result != mojom::Result::OK) {
    BLOG(0, "Order not completed");
    callback(result);
    return;
  }

  auto save_callback = std::bind(&ContributionSKU::CredsStepSaved, this, _1,
                                 contribution_id, callback);

  engine_->database()->UpdateContributionInfoStep(
      contribution_id, mojom::ContributionStep::STEP_CREDS, save_callback);
}

void ContributionSKU::CredsStepSaved(mojom::Result result,
                                     const std::string& contribution_id,
                                     LegacyResultCallback callback) {
  if (result != mojom::Result::OK) {
    BLOG(0, "Creds step not saved");
    callback(result);
    return;
  }

  engine_->contribution()->StartUnblinded({mojom::CredsBatchType::SKU},
                                          contribution_id, callback);
}

void ContributionSKU::Merchant(const mojom::SKUTransaction& transaction,
                               LegacyResultCallback callback) {
  auto get_callback = std::bind(&ContributionSKU::GetUnblindedTokens, this, _1,
                                transaction, callback);

  engine_->database()->GetSpendableUnblindedTokensByBatchTypes(
      {mojom::CredsBatchType::PROMOTION}, get_callback);
}

void ContributionSKU::GetUnblindedTokens(
    std::vector<mojom::UnblindedTokenPtr> list,
    const mojom::SKUTransaction& transaction,
    LegacyResultCallback callback) {
  if (list.empty()) {
    BLOG(0, "List is empty");
    callback(mojom::Result::FAILED);
    return;
  }

  std::vector<mojom::UnblindedToken> token_list;
  double current_amount = 0.0;
  for (auto& item : list) {
    if (current_amount >= transaction.amount) {
      break;
    }

    current_amount += item->value;
    token_list.push_back(*item);
  }

  if (current_amount < transaction.amount) {
    BLOG(0, "Not enough funds");
    callback(mojom::Result::NOT_ENOUGH_FUNDS);
    return;
  }

  credential::CredentialsRedeem redeem;
  redeem.type = mojom::RewardsType::PAYMENT;
  redeem.processor = mojom::ContributionProcessor::BRAVE_TOKENS;
  redeem.token_list = token_list;
  redeem.order_id = transaction.order_id;

  auto get_callback =
      std::bind(&ContributionSKU::GetOrderMerchant, this, _1, redeem, callback);

  engine_->database()->GetSKUOrder(transaction.order_id, get_callback);
}

void ContributionSKU::GetOrderMerchant(
    mojom::SKUOrderPtr order,
    const credential::CredentialsRedeem& redeem,
    LegacyResultCallback callback) {
  if (!order) {
    BLOG(0, "Order was not found");
    callback(mojom::Result::FAILED);
    return;
  }

  credential::CredentialsRedeem new_redeem = redeem;
  new_redeem.publisher_key = order->location;

  auto creds_callback =
      std::bind(&ContributionSKU::OnRedeemTokens, this, _1, callback);

  credentials_.RedeemTokens(new_redeem, creds_callback);
}

void ContributionSKU::OnRedeemTokens(mojom::Result result,
                                     LegacyResultCallback callback) {
  if (result != mojom::Result::OK) {
    BLOG(0, "Problem redeeming tokens");
    callback(result);
    return;
  }

  callback(result);
}

void ContributionSKU::Retry(mojom::ContributionInfoPtr contribution,
                            LegacyResultCallback callback) {
  if (!contribution) {
    BLOG(0, "Contribution was not found");
    callback(mojom::Result::FAILED);
    return;
  }

  auto get_callback = std::bind(
      &ContributionSKU::OnOrder, this, _1,
      std::make_shared<mojom::ContributionInfoPtr>(contribution->Clone()),
      callback);

  engine_->database()->GetSKUOrderByContributionId(
      contribution->contribution_id, get_callback);
}

void ContributionSKU::OnOrder(
    mojom::SKUOrderPtr order,
    std::shared_ptr<mojom::ContributionInfoPtr> shared_contribution,
    LegacyResultCallback callback) {
  auto contribution = std::move(*shared_contribution);
  if (!contribution) {
    BLOG(0, "Contribution is null");
    callback(mojom::Result::FAILED);
    return;
  }

  switch (contribution->step) {
    case mojom::ContributionStep::STEP_START:
    case mojom::ContributionStep::STEP_EXTERNAL_TRANSACTION: {
      RetryStartStep(std::move(contribution), std::move(order), callback);
      return;
    }
    case mojom::ContributionStep::STEP_PREPARE:
    case mojom::ContributionStep::STEP_RESERVE:
    case mojom::ContributionStep::STEP_CREDS: {
      engine_->contribution()->RetryUnblinded({mojom::CredsBatchType::SKU},
                                              contribution->contribution_id,
                                              callback);
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
      BLOG(0, "Step not correct " << contribution->step);
      NOTREACHED();
      return;
    }
  }
}

void ContributionSKU::RetryStartStep(mojom::ContributionInfoPtr contribution,
                                     mojom::SKUOrderPtr order,
                                     LegacyResultCallback callback) {
  if (!contribution) {
    BLOG(0, "Contribution is null");
    callback(mojom::Result::FAILED);
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
    BLOG(0, "Invalid processor for SKU contribution");
    callback(mojom::Result::FAILED);
    return;
  }

  // If an SKU order has not been created yet, then start the SKU order process
  // from the beginning.
  if (!order || order->order_id.empty()) {
    DCHECK(wallet_type == constant::kWalletUphold ||
           wallet_type == constant::kWalletGemini);
    AutoContribution(contribution->contribution_id, wallet_type, callback);
    return;
  }

  LegacyResultCallback complete_callback =
      std::bind(&ContributionSKU::Completed, this, _1,
                contribution->contribution_id, contribution->type, callback);

  auto retry_callback =
      std::bind(&ContributionSKU::GetOrder, this, _1, _2,
                contribution->contribution_id, complete_callback);

  sku_.Retry(order->order_id, wallet_type, retry_callback);
}

}  // namespace contribution
}  // namespace brave_rewards::internal
