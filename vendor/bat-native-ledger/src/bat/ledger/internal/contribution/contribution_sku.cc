/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/contribution/contribution_sku.h"
#include "bat/ledger/internal/contribution/contribution_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/sku/sku_brave.h"
#include "bat/ledger/internal/constants.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace {

const char kACSKUDev[] = "AgEJYnJhdmUuY29tAiNicmF2ZSB1c2VyLXdhbGxldC12b3RlIHNrdSB0b2tlbiB2MQACFHNrdT11c2VyLXdhbGxldC12b3RlAAIKcHJpY2U9MC4yNQACDGN1cnJlbmN5PUJBVAACDGRlc2NyaXB0aW9uPQACGmNyZWRlbnRpYWxfdHlwZT1zaW5nbGUtdXNlAAAGINiB9dUmpqLyeSEdZ23E4dPXwIBOUNJCFN9d5toIME2M";  //NOLINT
const char kACSKUStaging[] = "AgEJYnJhdmUuY29tAiNicmF2ZSB1c2VyLXdhbGxldC12b3RlIHNrdSB0b2tlbiB2MQACFHNrdT11c2VyLXdhbGxldC12b3RlAAIKcHJpY2U9MC4yNQACDGN1cnJlbmN5PUJBVAACDGRlc2NyaXB0aW9uPQACGmNyZWRlbnRpYWxfdHlwZT1zaW5nbGUtdXNlAAAGIOH4Li+rduCtFOfV8Lfa2o8h4SQjN5CuIwxmeQFjOk4W";  //NOLINT
const char kACSKUProduction[] = "AgEJYnJhdmUuY29tAiNicmF2ZSB1c2VyLXdhbGxldC12b3RlIHNrdSB0b2tlbiB2MQACFHNrdT11c2VyLXdhbGxldC12b3RlAAIKcHJpY2U9MC4yNQACDGN1cnJlbmN5PUJBVAACDGRlc2NyaXB0aW9uPQACGmNyZWRlbnRpYWxfdHlwZT1zaW5nbGUtdXNlAAAGIOaNAUCBMKm0IaLqxefhvxOtAKB0OfoiPn0NPVfI602J";  //NOLINT

std::string GetACSKU() {
  if (ledger::_environment == ledger::type::Environment::PRODUCTION) {
    return kACSKUProduction;
  }

  if (ledger::_environment == ledger::type::Environment::STAGING) {
    return kACSKUStaging;
  }

  if (ledger::_environment == ledger::type::Environment::DEVELOPMENT) {
    return kACSKUDev;
  }

  NOTREACHED();
  return kACSKUDev;
}

void GetCredentialTrigger(
    ledger::type::SKUOrderPtr order,
    ledger::credential::CredentialsTrigger* trigger) {
  DCHECK(trigger);

  if (!order || order->items.size() != 1) {
    return;
  }

  std::vector<std::string> data;
  data.push_back(order->items[0]->order_item_id);
  data.push_back(std::to_string(static_cast<int>(order->items[0]->type)));

  trigger->id = order->order_id;
  trigger->size = order->items[0]->quantity;
  trigger->type = ledger::type::CredsBatchType::SKU;
  trigger->data = data;
}

}  // namespace

namespace ledger {
namespace contribution {

ContributionSKU::ContributionSKU(LedgerImpl* ledger) :
    ledger_(ledger) {
  DCHECK(ledger_);
  credentials_ = credential::CredentialsFactory::Create(
      ledger_,
      type::CredsBatchType::SKU);
  DCHECK(credentials_);
  sku_ = sku::SKUFactory::Create(
      ledger_,
      sku::SKUType::kBrave);
  DCHECK(sku_);
}

ContributionSKU::~ContributionSKU() = default;

void ContributionSKU::AutoContribution(const std::string& contribution_id,
                                       const std::string& wallet_type,
                                       ledger::LegacyResultCallback callback) {
  type::SKUOrderItem item;
  item.sku = GetACSKU();

  Start(contribution_id, item, wallet_type, callback);
}

void ContributionSKU::Start(const std::string& contribution_id,
                            const type::SKUOrderItem& item,
                            const std::string& wallet_type,
                            ledger::LegacyResultCallback callback) {
  auto get_callback = std::bind(&ContributionSKU::GetContributionInfo,
      this,
      _1,
      item,
      wallet_type,
      callback);

  ledger_->database()->GetContributionInfo(contribution_id, get_callback);
}

void ContributionSKU::GetContributionInfo(
    type::ContributionInfoPtr contribution,
    const type::SKUOrderItem& item,
    const std::string& wallet_type,
    ledger::LegacyResultCallback callback) {
  if (!contribution) {
    BLOG(0, "Contribution not found");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  ledger::LegacyResultCallback complete_callback =
      std::bind(&ContributionSKU::Completed, this, _1,
                contribution->contribution_id, contribution->type, callback);

  auto process_callback = std::bind(&ContributionSKU::GetOrder,
      this,
      _1,
      _2,
      contribution->contribution_id,
      complete_callback);

  type::SKUOrderItem new_item = item;
  new_item.quantity = GetVotesFromAmount(contribution->amount);
  new_item.type = type::SKUOrderItemType::SINGLE_USE;
  new_item.price = constant::kVotePrice;

  std::vector<type::SKUOrderItem> items;
  items.push_back(new_item);

  sku_->Process(
      items,
      wallet_type,
      process_callback,
      contribution->contribution_id);
}

void ContributionSKU::GetOrder(type::Result result,
                               const std::string& order_id,
                               const std::string& contribution_id,
                               ledger::LegacyResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "SKU was not processed");
    callback(result);
    return;
  }

  auto get_callback = std::bind(&ContributionSKU::OnGetOrder,
      this,
      _1,
      contribution_id,
      callback);
  ledger_->database()->GetSKUOrder(order_id, get_callback);
}

void ContributionSKU::OnGetOrder(type::SKUOrderPtr order,
                                 const std::string& contribution_id,
                                 ledger::LegacyResultCallback callback) {
  if (!order) {
    BLOG(0, "Order was not found");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  DCHECK_EQ(order->items.size(), 1ul);
  credential::CredentialsTrigger trigger;
  GetCredentialTrigger(order->Clone(), &trigger);

  credentials_->Start(
      trigger, base::BindOnce([](ledger::LegacyResultCallback callback,
                                 type::Result result) { callback(result); },
                              std::move(callback)));
}

void ContributionSKU::Completed(type::Result result,
                                const std::string& contribution_id,
                                type::RewardsType type,
                                ledger::LegacyResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Order not completed");
    callback(result);
    return;
  }

  auto save_callback = std::bind(&ContributionSKU::CredsStepSaved,
      this,
      _1,
      contribution_id,
      callback);

  ledger_->database()->UpdateContributionInfoStep(
      contribution_id,
      type::ContributionStep::STEP_CREDS,
      save_callback);
}

void ContributionSKU::CredsStepSaved(type::Result result,
                                     const std::string& contribution_id,
                                     ledger::LegacyResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Creds step not saved");
    callback(result);
    return;
  }

  ledger_->contribution()->StartUnblinded(
      {type::CredsBatchType::SKU},
      contribution_id,
      callback);
}

void ContributionSKU::Merchant(
    const type::SKUTransaction& transaction,
    client::TransactionCallback callback) {
  auto get_callback = std::bind(&ContributionSKU::GetUnblindedTokens,
      this,
      _1,
      transaction,
      callback);

  ledger_->database()->GetSpendableUnblindedTokensByBatchTypes(
      {type::CredsBatchType::PROMOTION},
      get_callback);
}

void ContributionSKU::GetUnblindedTokens(
    type::UnblindedTokenList list,
    const type::SKUTransaction& transaction,
    client::TransactionCallback callback) {
  if (list.empty()) {
    BLOG(0, "List is empty");
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  std::vector<type::UnblindedToken> token_list;
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
    callback(type::Result::NOT_ENOUGH_FUNDS, "");
    return;
  }

  credential::CredentialsRedeem redeem;
  redeem.type = type::RewardsType::PAYMENT;
  redeem.processor = type::ContributionProcessor::BRAVE_TOKENS;
  redeem.token_list = token_list;
  redeem.order_id = transaction.order_id;

  auto get_callback = std::bind(&ContributionSKU::GetOrderMerchant,
      this,
      _1,
      redeem,
      callback);

  ledger_->database()->GetSKUOrder(transaction.order_id, get_callback);
}

void ContributionSKU::GetOrderMerchant(
    type::SKUOrderPtr order,
    const credential::CredentialsRedeem& redeem,
    client::TransactionCallback callback) {
  if (!order) {
    BLOG(0, "Order was not found");
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  credential::CredentialsRedeem new_redeem = redeem;
  new_redeem.publisher_key = order->location;

  auto creds_callback = std::bind(&ContributionSKU::OnRedeemTokens,
      this,
      _1,
      callback);

  credentials_->RedeemTokens(new_redeem, creds_callback);
}

void ContributionSKU::OnRedeemTokens(
    const type::Result result,
    client::TransactionCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Problem redeeming tokens");
    callback(result, "");
    return;
  }

  callback(result, "");
}

void ContributionSKU::Retry(type::ContributionInfoPtr contribution,
                            ledger::LegacyResultCallback callback) {
  if (!contribution) {
    BLOG(0, "Contribution was not found");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback = std::bind(&ContributionSKU::OnOrder,
      this,
      _1,
      std::make_shared<type::ContributionInfoPtr>(contribution->Clone()),
      callback);

  ledger_->database()->GetSKUOrderByContributionId(
      contribution->contribution_id,
      get_callback);
}

void ContributionSKU::OnOrder(
    type::SKUOrderPtr order,
    std::shared_ptr<type::ContributionInfoPtr> shared_contribution,
    ledger::LegacyResultCallback callback) {
  auto contribution = std::move(*shared_contribution);
  if (!contribution) {
    BLOG(0, "Contribution is null");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  switch (contribution->step) {
    case type::ContributionStep::STEP_START:
    case type::ContributionStep::STEP_EXTERNAL_TRANSACTION: {
      RetryStartStep(std::move(contribution), std::move(order), callback);
      return;
    }
    case type::ContributionStep::STEP_PREPARE:
    case type::ContributionStep::STEP_RESERVE:
    case type::ContributionStep::STEP_CREDS: {
      ledger_->contribution()->RetryUnblinded(
          {type::CredsBatchType::SKU},
          contribution->contribution_id,
          callback);
      return;
    }
    case type::ContributionStep::STEP_RETRY_COUNT:
    case type::ContributionStep::STEP_REWARDS_OFF:
    case type::ContributionStep::STEP_AC_OFF:
    case type::ContributionStep::STEP_AC_TABLE_EMPTY:
    case type::ContributionStep::STEP_NOT_ENOUGH_FUNDS:
    case type::ContributionStep::STEP_FAILED:
    case type::ContributionStep::STEP_COMPLETED:
    case type::ContributionStep::STEP_NO: {
      BLOG(0, "Step not correct " << contribution->step);
      NOTREACHED();
      return;
    }
  }
}

void ContributionSKU::RetryStartStep(type::ContributionInfoPtr contribution,
                                     type::SKUOrderPtr order,
                                     ledger::LegacyResultCallback callback) {
  if (!contribution) {
    BLOG(0, "Contribution is null");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  std::string wallet_type;
  switch (contribution->processor) {
    case type::ContributionProcessor::UPHOLD:
      wallet_type = constant::kWalletUphold;
      break;
    case type::ContributionProcessor::GEMINI:
      wallet_type = constant::kWalletGemini;
      break;
    case type::ContributionProcessor::NONE:
    case type::ContributionProcessor::BRAVE_TOKENS:
    case type::ContributionProcessor::BITFLYER:
      break;
  }

  if (wallet_type.empty()) {
    BLOG(0, "Invalid processor for SKU contribution");
    callback(type::Result::LEDGER_ERROR);
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

  ledger::LegacyResultCallback complete_callback =
      std::bind(&ContributionSKU::Completed, this, _1,
                contribution->contribution_id, contribution->type, callback);

  auto retry_callback =
      std::bind(&ContributionSKU::GetOrder, this, _1, _2,
                contribution->contribution_id, complete_callback);

  sku_->Retry(order->order_id, wallet_type, retry_callback);
}

}  // namespace contribution
}  // namespace ledger
