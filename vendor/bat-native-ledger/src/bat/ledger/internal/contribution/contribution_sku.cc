/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/common/bind_util.h"
#include "bat/ledger/internal/contribution/contribution_sku.h"
#include "bat/ledger/internal/contribution/contribution_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/sku/sku_brave.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/internal/uphold/uphold_util.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace {

const char kACSKUDev[] = "AgEJYnJhdmUuY29tAiNicmF2ZSB1c2VyLXdhbGxldC12b3RlIHNrdSB0b2tlbiB2MQACFHNrdT11c2VyLXdhbGxldC12b3RlAAIKcHJpY2U9MC4yNQACDGN1cnJlbmN5PUJBVAACDGRlc2NyaXB0aW9uPQACGmNyZWRlbnRpYWxfdHlwZT1zaW5nbGUtdXNlAAAGINiB9dUmpqLyeSEdZ23E4dPXwIBOUNJCFN9d5toIME2M";  //NOLINT
const char kACSKUStaging[] = "AgEJYnJhdmUuY29tAiNicmF2ZSB1c2VyLXdhbGxldC12b3RlIHNrdSB0b2tlbiB2MQACFHNrdT11c2VyLXdhbGxldC12b3RlAAIKcHJpY2U9MC4yNQACDGN1cnJlbmN5PUJBVAACDGRlc2NyaXB0aW9uPQACGmNyZWRlbnRpYWxfdHlwZT1zaW5nbGUtdXNlAAAGIOH4Li+rduCtFOfV8Lfa2o8h4SQjN5CuIwxmeQFjOk4W";  //NOLINT
const char kACSKUProduction[] = "AgEJYnJhdmUuY29tAiNicmF2ZSB1c2VyLXdhbGxldC12b3RlIHNrdSB0b2tlbiB2MQACFHNrdT11c2VyLXdhbGxldC12b3RlAAIKcHJpY2U9MC4yNQACDGN1cnJlbmN5PUJBVAACDGRlc2NyaXB0aW9uPQACGmNyZWRlbnRpYWxfdHlwZT1zaW5nbGUtdXNlAAAGIOaNAUCBMKm0IaLqxefhvxOtAKB0OfoiPn0NPVfI602J";  //NOLINT

const char kUserFundsSKUDev[] = "AgEJYnJhdmUuY29tAiFicmF2ZSBhbm9uLWNhcmQtdm90ZSBza3UgdG9rZW4gdjEAAhJza3U9YW5vbi1jYXJkLXZvdGUAAgpwcmljZT0wLjI1AAIMY3VycmVuY3k9QkFUAAIMZGVzY3JpcHRpb249AAIaY3JlZGVudGlhbF90eXBlPXNpbmdsZS11c2UAAAYgPpv+Al9jRgVCaR49/AoRrsjQqXGqkwaNfqVka00SJxQ=";  //NOLINT
const char kUserFundsSKUStaging[] = "AgEJYnJhdmUuY29tAiFicmF2ZSBhbm9uLWNhcmQtdm90ZSBza3UgdG9rZW4gdjEAAhJza3U9YW5vbi1jYXJkLXZvdGUAAgpwcmljZT0wLjI1AAIMY3VycmVuY3k9QkFUAAIMZGVzY3JpcHRpb249AAIaY3JlZGVudGlhbF90eXBlPXNpbmdsZS11c2UAAAYgPV/WYY5pXhodMPvsilnrLzNH6MA8nFXwyg0qSWX477M=";  //NOLINT
const char kUserFundsSKUProduction[] = "AgEJYnJhdmUuY29tAiFicmF2ZSBhbm9uLWNhcmQtdm90ZSBza3UgdG9rZW4gdjEAAhJza3U9YW5vbi1jYXJkLXZvdGUAAgpwcmljZT0wLjI1AAIMY3VycmVuY3k9QkFUAAIMZGVzY3JpcHRpb249AAIaY3JlZGVudGlhbF90eXBlPXNpbmdsZS11c2UAAAYgrMZm85YYwnmjPXcegy5pBM5C+ZLfrySZfYiSe13yp8o=";  //NOLINT

std::string GetACSKU() {
  if (ledger::_environment == ledger::Environment::PRODUCTION) {
    return kACSKUProduction;
  }

  if (ledger::_environment == ledger::Environment::STAGING) {
    return kACSKUStaging;
  }

  if (ledger::_environment == ledger::Environment::DEVELOPMENT) {
    return kACSKUDev;
  }

  NOTREACHED();
  return kACSKUDev;
}

std::string GetUserFundsSKU() {
  if (ledger::_environment == ledger::Environment::PRODUCTION) {
    return kUserFundsSKUProduction;
  }

  if (ledger::_environment == ledger::Environment::STAGING) {
    return kUserFundsSKUStaging;
  }

  if (ledger::_environment == ledger::Environment::DEVELOPMENT) {
    return kUserFundsSKUDev;
  }

  NOTREACHED();
  return kUserFundsSKUDev;
}

void GetCredentialTrigger(
    ledger::SKUOrderPtr order,
    braveledger_credentials::CredentialsTrigger* trigger) {
  DCHECK(trigger);

  if (!order || order->items.size() != 1) {
    return;
  }

  std::vector<std::string> data;
  data.push_back(order->items[0]->order_item_id);
  data.push_back(std::to_string(static_cast<int>(order->items[0]->type)));

  trigger->id = order->order_id;
  trigger->size = order->items[0]->quantity;
  trigger->type = ledger::CredsBatchType::SKU;
  trigger->data = data;
}

}  // namespace

namespace braveledger_contribution {

ContributionSKU::ContributionSKU(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
  DCHECK(ledger_);
  credentials_ = braveledger_credentials::CredentialsFactory::Create(
      ledger_,
      ledger::CredsBatchType::SKU);
  DCHECK(credentials_);
  sku_ = braveledger_sku::SKUFactory::Create(
      ledger_,
      braveledger_sku::SKUType::kBrave);
  DCHECK(sku_);
}

ContributionSKU::~ContributionSKU() = default;

void ContributionSKU::AutoContribution(
    const std::string& contribution_id,
    ledger::ExternalWalletPtr wallet,
    ledger::ResultCallback callback) {
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  ledger::SKUOrderItem item;
  item.sku = GetACSKU();

  Start(
      contribution_id,
      item,
      std::move(wallet),
      callback);
}

void ContributionSKU::AnonUserFunds(
    const std::string& contribution_id,
    ledger::ExternalWalletPtr wallet,
    ledger::ResultCallback callback) {
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  ledger::SKUOrderItem item;
  item.sku = GetUserFundsSKU();

  Start(
      contribution_id,
      item,
      std::move(wallet),
      callback);
}

void ContributionSKU::Start(
    const std::string& contribution_id,
    const ledger::SKUOrderItem& item,
    ledger::ExternalWalletPtr wallet,
    ledger::ResultCallback callback) {
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback = std::bind(&ContributionSKU::GetContributionInfo,
      this,
      _1,
      item,
      *wallet,
      callback);

  ledger_->database()->GetContributionInfo(contribution_id, get_callback);
}

void ContributionSKU::GetContributionInfo(
    ledger::ContributionInfoPtr contribution,
    const ledger::SKUOrderItem& item,
    const ledger::ExternalWallet& wallet,
    ledger::ResultCallback callback) {
  if (!contribution) {
    BLOG(0, "Contribution not found");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  ledger::ResultCallback complete_callback = std::bind(
      &ContributionSKU::Completed,
      this,
      _1,
      contribution->contribution_id,
      contribution->type,
      callback);

  auto process_callback = std::bind(&ContributionSKU::GetOrder,
      this,
      _1,
      _2,
      contribution->contribution_id,
      complete_callback);

  ledger::SKUOrderItem new_item = item;
  new_item.quantity = GetVotesFromAmount(contribution->amount);
  new_item.type = ledger::SKUOrderItemType::SINGLE_USE;
  new_item.price = braveledger_ledger::_vote_price;

  std::vector<ledger::SKUOrderItem> items;
  items.push_back(new_item);

  sku_->Process(
      items,
      ledger::ExternalWallet::New(wallet),
      process_callback,
      contribution->contribution_id);
}

void ContributionSKU::GetOrder(
    const ledger::Result result,
    const std::string& order_id,
    const std::string& contribution_id,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
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

void ContributionSKU::OnGetOrder(
    ledger::SKUOrderPtr order,
    const std::string& contribution_id,
    ledger::ResultCallback callback) {
  if (!order) {
    BLOG(0, "Order was not found");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto save_callback = std::bind(&ContributionSKU::TransactionStepSaved,
      this,
      _1,
      braveledger_bind_util::FromSKUOrderToString(std::move(order)),
      callback);

  ledger_->database()->UpdateContributionInfoStep(
      contribution_id,
      ledger::ContributionStep::STEP_EXTERNAL_TRANSACTION,
      save_callback);
}

void ContributionSKU::TransactionStepSaved(
    const ledger::Result result,
    const std::string& order_string,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "External transaction step was not saved");
    callback(result);
    return;
  }

  auto order = braveledger_bind_util::FromStringToSKUOrder(order_string);
  if (!order) {
    BLOG(0, "Order is corrupted");
    callback(ledger::Result::RETRY);
    return;
  }

  DCHECK_EQ(order->items.size(), 1ul);
  braveledger_credentials::CredentialsTrigger trigger;
  GetCredentialTrigger(order->Clone(), &trigger);

  credentials_->Start(trigger, callback);
}

void ContributionSKU::Completed(
    const ledger::Result result,
    const std::string& contribution_id,
    const ledger::RewardsType type,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
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
      ledger::ContributionStep::STEP_CREDS,
      save_callback);
}

void ContributionSKU::CredsStepSaved(
    const ledger::Result result,
    const std::string& contribution_id,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Creds step not saved");
    callback(result);
    return;
  }

  ledger_->contribution()->StartUnblinded(
      {ledger::CredsBatchType::SKU},
      contribution_id,
      callback);
}

void ContributionSKU::Merchant(
    const ledger::SKUTransaction& transaction,
    ledger::TransactionCallback callback) {
  auto get_callback = std::bind(&ContributionSKU::GetUnblindedTokens,
      this,
      _1,
      transaction,
      callback);

  ledger_->database()->GetSpendableUnblindedTokensByBatchTypes(
      {ledger::CredsBatchType::PROMOTION},
      get_callback);
}

void ContributionSKU::GetUnblindedTokens(
    ledger::UnblindedTokenList list,
    const ledger::SKUTransaction& transaction,
    ledger::TransactionCallback callback) {
  if (list.empty()) {
    BLOG(0, "List is empty");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  std::vector<ledger::UnblindedToken> token_list;
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
    callback(ledger::Result::NOT_ENOUGH_FUNDS, "");
    return;
  }

  braveledger_credentials::CredentialsRedeem redeem;
  redeem.type = ledger::RewardsType::PAYMENT;
  redeem.processor = ledger::ContributionProcessor::BRAVE_TOKENS;
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
    ledger::SKUOrderPtr order,
    const braveledger_credentials::CredentialsRedeem& redeem,
    ledger::TransactionCallback callback) {
  if (!order) {
    BLOG(0, "Order was not found");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  braveledger_credentials::CredentialsRedeem new_redeem = redeem;
  new_redeem.publisher_key = order->location;

  auto creds_callback = std::bind(&ContributionSKU::OnRedeemTokens,
      this,
      _1,
      callback);

  credentials_->RedeemTokens(new_redeem, creds_callback);
}

void ContributionSKU::OnRedeemTokens(
    const ledger::Result result,
    ledger::TransactionCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Problem redeeming tokens");
    callback(result, "");
    return;
  }

  callback(result, "");
}

void ContributionSKU::Retry(
    const ledger::ContributionInfoPtr contribution,
    ledger::ResultCallback callback) {
  if (!contribution) {
    BLOG(0, "Contribution was not found");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback = std::bind(&ContributionSKU::OnOrder,
      this,
      _1,
      braveledger_bind_util::FromContributionToString(contribution->Clone()),
      callback);

  ledger_->database()->GetSKUOrderByContributionId(
      contribution->contribution_id,
      get_callback);
}

void ContributionSKU::OnOrder(
    ledger::SKUOrderPtr order,
    const std::string& contribution_string,
    ledger::ResultCallback callback) {
  auto contribution = braveledger_bind_util::FromStringToContribution(
      contribution_string);

  if (!contribution) {
    BLOG(0, "Contribution is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  switch (contribution->step) {
    case ledger::ContributionStep::STEP_START: {
      RetryStartStep(
          std::move(contribution),
          std::move(order),
          callback);
      return;
    }
    case ledger::ContributionStep::STEP_EXTERNAL_TRANSACTION: {
      RetryExternalTransactionStep(
          std::move(contribution),
          std::move(order),
          callback);
      return;
    }
    case ledger::ContributionStep::STEP_PREPARE:
    case ledger::ContributionStep::STEP_RESERVE:
    case ledger::ContributionStep::STEP_CREDS: {
      ledger_->contribution()->RetryUnblinded(
          {ledger::CredsBatchType::SKU},
          contribution->contribution_id,
          callback);
      return;
    }
    case ledger::ContributionStep::STEP_RETRY_COUNT:
    case ledger::ContributionStep::STEP_REWARDS_OFF:
    case ledger::ContributionStep::STEP_AC_OFF:
    case ledger::ContributionStep::STEP_AC_TABLE_EMPTY:
    case ledger::ContributionStep::STEP_NOT_ENOUGH_FUNDS:
    case ledger::ContributionStep::STEP_FAILED:
    case ledger::ContributionStep::STEP_COMPLETED:
    case ledger::ContributionStep::STEP_NO: {
      BLOG(0, "Step not correct " << contribution->step);
      NOTREACHED();
      return;
    }
  }
}

void ContributionSKU::RetryStartStep(
    ledger::ContributionInfoPtr contribution,
    ledger::SKUOrderPtr order,
    ledger::ResultCallback callback) {
  if (!contribution) {
    BLOG(0, "Contribution is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (contribution->processor == ledger::ContributionProcessor::UPHOLD &&
      contribution->type == ledger::RewardsType::AUTO_CONTRIBUTE) {
    std::string order_id;
    if (order) {
      order_id = order->order_id;
    }

    auto get_callback = std::bind(
        &ContributionSKU::RetryStartStepExternalWallet,
        this,
        _1,
        _2,
        order_id,
        contribution->contribution_id,
        callback);
    ledger_->wallet()->GetExternalWallet(ledger::kWalletUphold, get_callback);
    return;
  }

  auto wallet = ledger::ExternalWallet::New();
  wallet->type = ledger::kWalletAnonymous;

  if (!order) {
    AnonUserFunds(contribution->contribution_id, std::move(wallet), callback);
    return;
  }

  auto retry_callback = std::bind(&ContributionSKU::GetOrder,
      this,
      _1,
      _2,
      contribution->contribution_id,
      callback);

  sku_->Retry(order->order_id, std::move(wallet), retry_callback);
}

void ContributionSKU::RetryStartStepExternalWallet(
    const ledger::Result result,
    ledger::ExternalWalletPtr wallet,
    const std::string& order_id,
    const std::string& contribution_id,
    ledger::ResultCallback callback) {
  if (!wallet || result != ledger::Result::LEDGER_OK) {
    BLOG(0, "External wallet is missing");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (order_id.empty()) {
    AutoContribution(contribution_id, std::move(wallet), callback);
    return;
  }

  auto retry_callback = std::bind(&ContributionSKU::GetOrder,
      this,
      _1,
      _2,
      contribution_id,
      callback);

  sku_->Retry(order_id, std::move(wallet), retry_callback);
}

void ContributionSKU::RetryExternalTransactionStep(
    ledger::ContributionInfoPtr contribution,
    ledger::SKUOrderPtr order,
    ledger::ResultCallback callback) {
  if (!contribution || !order) {
    BLOG(0, "Contribution/order is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  braveledger_credentials::CredentialsTrigger trigger;
  GetCredentialTrigger(order->Clone(), &trigger);
  auto complete_callback = std::bind(&ContributionSKU::Completed,
      this,
      _1,
      contribution->contribution_id,
      contribution->type,
      callback);
  credentials_->Start(trigger, complete_callback);
}

}  // namespace braveledger_contribution
