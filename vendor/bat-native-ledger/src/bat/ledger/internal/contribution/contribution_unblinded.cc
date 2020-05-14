/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "bat/ledger/internal/bat_util.h"
#include "bat/ledger/internal/common/bind_util.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/contribution/contribution_sku.h"
#include "bat/ledger/internal/contribution/contribution_unblinded.h"
#include "bat/ledger/internal/contribution/contribution_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/request/request_promotion.h"
#include "brave_base/random.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace {

bool GetStatisticalVotingWinner(
    double dart,
    const double amount,
    const std::vector<ledger::ContributionPublisher>& list,
    braveledger_contribution::Winners* winners) {
  DCHECK(winners);

  double upper = 0.0;
  for (auto& item : list) {
    upper += item.total_amount / amount;
    if (upper < dart) {
      continue;
    }

    auto iter = winners->find(item.publisher_key);

    uint32_t current_value = 0;
    if (iter != winners->end()) {
      current_value = winners->at(item.publisher_key);
      winners->at(item.publisher_key) = current_value + 1;
    } else {
      winners->emplace(item.publisher_key, 1);
    }

    return true;
  }

  return false;
}

void GetStatisticalVotingWinners(
    uint32_t total_votes,
    const double amount,
    const ledger::ContributionPublisherList& list,
    braveledger_contribution::Winners* winners) {
  DCHECK(winners);
  std::vector<ledger::ContributionPublisher> converted_list;

  if (total_votes == 0 || list.empty()) {
    return;
  }

  for (auto& item : list) {
    ledger::ContributionPublisher new_item;
    new_item.total_amount = item->total_amount;
    new_item.publisher_key = item->publisher_key;
    converted_list.push_back(new_item);
  }

  while (total_votes > 0) {
    double dart = brave_base::random::Uniform_01();
    if (GetStatisticalVotingWinner(dart, amount, converted_list, winners)) {
      --total_votes;
    }
  }
}

}  // namespace

namespace braveledger_contribution {

Unblinded::Unblinded(bat_ledger::LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
  credentials_promotion_ = braveledger_credentials::CredentialsFactory::Create(
      ledger_,
      ledger::CredsBatchType::PROMOTION);
  credentials_sku_ = braveledger_credentials::CredentialsFactory::Create(
      ledger_,
      ledger::CredsBatchType::SKU);
  DCHECK(credentials_promotion_ && credentials_sku_);
}

Unblinded::~Unblinded() = default;

void Unblinded::Start(
    const std::vector<ledger::CredsBatchType>& types,
    const std::string& contribution_id,
    ledger::ResultCallback callback) {
  if (contribution_id.empty()) {
    BLOG(0, "Contribution id is empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback = std::bind(&Unblinded::PrepareTokens,
      this,
      _1,
      _2,
      types,
      callback);

  GetContributionInfoAndUnblindedTokens(types, contribution_id, get_callback);
}

void Unblinded::GetContributionInfoAndUnblindedTokens(
    const std::vector<ledger::CredsBatchType>& types,
    const std::string& contribution_id,
    GetContributionInfoAndUnblindedTokensCallback callback) {
  auto get_callback = std::bind(&Unblinded::OnUnblindedTokens,
      this,
      _1,
      contribution_id,
      callback);
  ledger_->GetSpendableUnblindedTokensByBatchTypes(types, get_callback);
}

void Unblinded::OnUnblindedTokens(
    ledger::UnblindedTokenList list,
    const std::string& contribution_id,
    GetContributionInfoAndUnblindedTokensCallback callback) {
  BLOG_IF(1, list.empty(), "Token list is empty");

  std::vector<ledger::UnblindedToken> converted_list;
  for (const auto& item : list) {
    ledger::UnblindedToken new_item;
    new_item.id = item->id;
    new_item.token_value = item->token_value;
    new_item.public_key = item->public_key;
    new_item.value = item->value;
    new_item.creds_id = item->creds_id;
    new_item.expires_at = item->expires_at;

    converted_list.push_back(new_item);
  }

  ledger_->GetContributionInfo(contribution_id,
      std::bind(&Unblinded::OnGetContributionInfo,
                this,
                _1,
                std::move(converted_list),
                callback));
}

void Unblinded::GetContributionInfoAndReservedUnblindedTokens(
    const std::string& contribution_id,
    GetContributionInfoAndUnblindedTokensCallback callback) {
  auto get_callback = std::bind(&Unblinded::OnReservedUnblindedTokens,
      this,
      _1,
      contribution_id,
      callback);
  ledger_->GetReservedUnblindedTokens(contribution_id, get_callback);
}

void Unblinded::OnReservedUnblindedTokens(
    ledger::UnblindedTokenList list,
    const std::string& contribution_id,
    GetContributionInfoAndUnblindedTokensCallback callback) {
  if (list.empty()) {
    BLOG(0, "Token list is empty");
    callback(nullptr, {});
    return;
  }

  std::vector<ledger::UnblindedToken> converted_list;
  for (const auto& item : list) {
    ledger::UnblindedToken new_item;
    new_item.id = item->id;
    new_item.token_value = item->token_value;
    new_item.public_key = item->public_key;
    new_item.value = item->value;
    new_item.creds_id = item->creds_id;
    new_item.expires_at = item->expires_at;

    converted_list.push_back(new_item);
  }

  ledger_->GetContributionInfo(contribution_id,
      std::bind(&Unblinded::OnGetContributionInfo,
                this,
                _1,
                converted_list,
                callback));
}

void Unblinded::OnGetContributionInfo(
    ledger::ContributionInfoPtr contribution,
    const std::vector<ledger::UnblindedToken>& list,
    GetContributionInfoAndUnblindedTokensCallback callback) {
  callback(std::move(contribution), list);
}

void Unblinded::PrepareTokens(
    ledger::ContributionInfoPtr contribution,
    const std::vector<ledger::UnblindedToken>& list,
    const std::vector<ledger::CredsBatchType>& types,
    ledger::ResultCallback callback) {
  if (!contribution) {
    BLOG(0, "Contribution not found");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (list.empty()) {
    BLOG(0, "Not enough funds");
    callback(ledger::Result::NOT_ENOUGH_FUNDS);
    return;
  }

  double current_amount = 0.0;
  std::vector<ledger::UnblindedToken> token_list;
  for (const auto& item : list) {
    if (current_amount >= contribution->amount) {
      break;
    }

    current_amount += item.value;
    token_list.push_back(item);
  }

  if (current_amount < contribution->amount) {
    callback(ledger::Result::NOT_ENOUGH_FUNDS);
    BLOG(0, "Not enough funds");
    return;
  }

  const std::string contribution_id = contribution->contribution_id;
  const std::string contribution_string =
      braveledger_bind_util::FromContributionToString(std::move(contribution));

  std::vector<std::string> token_id_list;
  for (const auto& item : token_list) {
    token_id_list.push_back(base::NumberToString(item.id));
  }

  auto reserved_callback = std::bind(
      &Unblinded::OnMarkUnblindedTokensAsReserved,
      this,
      _1,
      std::move(token_list),
      contribution_string,
      types,
      callback);

  ledger_->MarkUnblindedTokensAsReserved(
      token_id_list,
      contribution_id,
      reserved_callback);
}

void Unblinded::ReserveStepSaved(
    const ledger::Result result,
    const std::vector<ledger::UnblindedToken>& list,
    const std::string& contribution_string,
    const std::vector<ledger::CredsBatchType>& types,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Reserve step was not saved");
    callback(ledger::Result::RETRY);
    return;
  }

  auto contribution =
      braveledger_bind_util::FromStringToContribution(contribution_string);
  if (!contribution) {
    BLOG(0, "Contribution was not converted successfully");
    callback(ledger::Result::RETRY);
    return;
  }

  // Discard cached reserved tokens now that retry window has passed
  DiscardReservedUnblindedTokensFromCache(contribution->contribution_id);

  PreparePublishers(list, std::move(contribution), types, callback);
}

void Unblinded::OnMarkUnblindedTokensAsReserved(
    const ledger::Result result,
    const std::vector<ledger::UnblindedToken>& list,
    const std::string& contribution_string,
    const std::vector<ledger::CredsBatchType>& types,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Failed to reserve unblinded tokens");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto contribution = braveledger_bind_util::FromStringToContribution(
      contribution_string);
  if (!contribution) {
    BLOG(0, "Contribution was not converted successfully");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const std::string contribution_id = contribution->contribution_id;

  // Cache reserved tokens for potential retry attempt
  AddReservedUnblindedTokensToCache(contribution_id, list);

  auto save_callback = std::bind(&Unblinded::ReserveStepSaved,
      this,
      _1,
      list,
      contribution_string,
      types,
      callback);

  ledger_->UpdateContributionInfoStep(
      contribution_id,
      ledger::ContributionStep::STEP_RESERVE,
      save_callback);
}

void Unblinded::PreparePublishers(
    const std::vector<ledger::UnblindedToken>& list,
    ledger::ContributionInfoPtr contribution,
    const std::vector<ledger::CredsBatchType>& types,
    ledger::ResultCallback callback) {
  if (!contribution) {
    BLOG(0, "Contribution not found");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (contribution->type == ledger::RewardsType::AUTO_CONTRIBUTE) {
    auto publisher_list =
        PrepareAutoContribution(list, contribution->Clone());

    if (publisher_list.empty()) {
      BLOG(0, "Publisher list empty");
      callback(ledger::Result::AC_TABLE_EMPTY);
      return;
    }

    contribution->publishers = std::move(publisher_list);

    auto save_callback = std::bind(&Unblinded::OnPrepareAutoContribution,
        this,
        _1,
        types,
        contribution->contribution_id,
        callback);

    ledger_->SaveContributionInfo(
      contribution->Clone(),
      save_callback);
    return;
  }

  auto save_callback = std::bind(&Unblinded::PrepareStepSaved,
      this,
      _1,
      types,
      contribution->contribution_id,
      callback);

  ledger_->UpdateContributionInfoStep(
      contribution->contribution_id,
      ledger::ContributionStep::STEP_PREPARE,
      save_callback);
}

ledger::ContributionPublisherList Unblinded::PrepareAutoContribution(
    const std::vector<ledger::UnblindedToken>& list,
    ledger::ContributionInfoPtr contribution) {
  if (!contribution) {
    BLOG(0, "Contribution is null");
    return {};
  }

  if (list.size() == 0) {
    BLOG(0, "Token list is empty");
    return {};
  }

  if (contribution->publishers.empty()) {
    BLOG(0, "Publisher list is empty");
    return {};
  }

  const double total_votes = static_cast<double>(list.size());
  Winners winners;
  GetStatisticalVotingWinners(
      total_votes,
      contribution->amount,
      std::move(contribution->publishers),
      &winners);

  ledger::ContributionPublisherList publisher_list;
  for (auto & winner : winners) {
    if (winner.second == 0) {
      continue;
    }

    const std::string publisher_key = winner.first;
    auto publisher = ledger::ContributionPublisher::New();
    publisher->contribution_id = contribution->contribution_id;
    publisher->publisher_key = publisher_key;
    publisher->total_amount =
        (winner.second / total_votes) * contribution->amount;
    publisher->contributed_amount = 0;
    publisher_list.push_back(std::move(publisher));
  }

  return publisher_list;
}

void Unblinded::OnPrepareAutoContribution(
    const ledger::Result result,
    const std::vector<ledger::CredsBatchType>& types,
    const std::string& contribution_id,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Contribution not saved");
    callback(ledger::Result::RETRY);
    return;
  }

  auto save_callback = std::bind(&Unblinded::PrepareStepSaved,
      this,
      _1,
      types,
      contribution_id,
      callback);

  ledger_->UpdateContributionInfoStep(
      contribution_id,
      ledger::ContributionStep::STEP_PREPARE,
      save_callback);
}

void Unblinded::PrepareStepSaved(
    const ledger::Result result,
    const std::vector<ledger::CredsBatchType>& types,
    const std::string& contribution_id,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Prepare step was not saved");
    callback(ledger::Result::RETRY);
    return;
  }

  ProcessTokens(types, contribution_id, callback);
}

void Unblinded::ProcessTokens(
    const std::vector<ledger::CredsBatchType>& types,
    const std::string& contribution_id,
    ledger::ResultCallback callback) {
  auto get_callback =  std::bind(&Unblinded::OnProcessTokens,
      this,
      _1,
      _2,
      callback);
  GetContributionInfoAndReservedUnblindedTokens(contribution_id, get_callback);
}

void Unblinded::OnProcessTokens(
    ledger::ContributionInfoPtr contribution,
    const std::vector<ledger::UnblindedToken>& list,
    ledger::ResultCallback callback) {
  if (!contribution || contribution->publishers.empty()) {
    BLOG(0, "Contribution not found");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  bool final_publisher = false;
  for (auto publisher = contribution->publishers.begin();
      publisher != contribution->publishers.end();
      publisher++) {
    if ((*publisher)->total_amount == (*publisher)->contributed_amount) {
      continue;
    }

    if (std::next(publisher) == contribution->publishers.end()) {
      final_publisher = true;
    }

    std::vector<ledger::UnblindedToken> token_list;
    double current_amount = 0.0;
    for (auto& item : list) {
      if (current_amount >= (*publisher)->total_amount) {
        break;
      }

      current_amount += item.value;
      token_list.push_back(item);
    }

    auto redeem_callback = std::bind(&Unblinded::TokenProcessed,
        this,
        _1,
        contribution->contribution_id,
        (*publisher)->publisher_key,
        final_publisher,
        callback);

    braveledger_credentials::CredentialsRedeem redeem;
    redeem.publisher_key = (*publisher)->publisher_key;
    redeem.type = contribution->type;
    redeem.processor = contribution->processor;
    redeem.token_list = token_list;
    redeem.contribution_id = contribution->contribution_id;

    if (redeem.processor == ledger::ContributionProcessor::UPHOLD ||
        redeem.processor == ledger::ContributionProcessor::BRAVE_USER_FUNDS) {
      credentials_sku_->RedeemTokens(redeem, redeem_callback);
      return;
    }

    credentials_promotion_->RedeemTokens(redeem, redeem_callback);
    return;
  }

  // we processed all publishers
  callback(ledger::Result::LEDGER_OK);
}

void Unblinded::TokenProcessed(
    const ledger::Result result,
    const std::string& contribution_id,
    const std::string& publisher_key,
    const bool final_publisher,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Tokens were not processed correctly");
    callback(ledger::Result::RETRY);
    return;
  }

  auto save_callback = std::bind(&Unblinded::ContributionAmountSaved,
      this,
      _1,
      contribution_id,
      final_publisher,
      callback);

  ledger_->UpdateContributionInfoContributedAmount(
      contribution_id,
      publisher_key,
      save_callback);
}

void Unblinded::ContributionAmountSaved(
    const ledger::Result result,
    const std::string& contribution_id,
    const bool final_publisher,
    ledger::ResultCallback callback) {
  if (final_publisher) {
    callback(result);
    return;
  }

  callback(ledger::Result::RETRY_LONG);
}

void Unblinded::Retry(
    const std::vector<ledger::CredsBatchType>& types,
    ledger::ContributionInfoPtr contribution,
    ledger::ResultCallback callback) {
  if (!contribution) {
    BLOG(0, "Contribution is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const bool is_not_tokens =
      contribution->processor != ledger::ContributionProcessor::BRAVE_TOKENS;

  const bool is_not_uphold_ac =
      contribution->processor == ledger::ContributionProcessor::UPHOLD &&
      contribution->type != ledger::RewardsType::AUTO_CONTRIBUTE;

  if (is_not_tokens && is_not_uphold_ac) {
    BLOG(0, "Retry is not for this func");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  switch (contribution->step) {
    case ledger::ContributionStep::STEP_START: {
      Start(types, contribution->contribution_id, callback);
      return;
    }
    case ledger::ContributionStep::STEP_PREPARE: {
      ProcessTokens(types, contribution->contribution_id, callback);
      return;
    }
    case ledger::ContributionStep::STEP_RESERVE: {
      // Retrieve reserved unblinded tokens from cache for retry
      // attempt
      std::vector<ledger::UnblindedToken> list;
      if (!GetReservedUnblindedTokensFromCache(
            contribution->contribution_id,
            &list)) {
        BLOG(0, "No reserved unblinded tokens saved for retry attempt");
        callback(ledger::Result::LEDGER_ERROR);
        return;
      }

      // Discard reserved unblinded tokens from cache
      DiscardReservedUnblindedTokensFromCache(contribution->contribution_id);

      PreparePublishers(list, std::move(contribution), types, callback);
      return;
    }
    case ledger::ContributionStep::STEP_AC_TABLE_EMPTY:
    case ledger::ContributionStep::STEP_CREDS:
    case ledger::ContributionStep::STEP_EXTERNAL_TRANSACTION:
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

void Unblinded::AddReservedUnblindedTokensToCache(
    const std::string& contribution_id,
    const std::vector<ledger::UnblindedToken>& list) {
  DCHECK(!contribution_id.empty());
  base::AutoLock lock(reserved_unblinded_tokens_lock_);
  reserved_unblinded_tokens_for_retry_.insert({contribution_id, list});
}

bool Unblinded::GetReservedUnblindedTokensFromCache(
    const std::string& contribution_id,
    std::vector<ledger::UnblindedToken>* list) {
  DCHECK(!contribution_id.empty());
  DCHECK(list);

  base::AutoLock lock(reserved_unblinded_tokens_lock_);
  auto iter = reserved_unblinded_tokens_for_retry_.find(contribution_id);
  if (iter == reserved_unblinded_tokens_for_retry_.end()) {
    return false;
  }

  *list = iter->second;

  return true;
}

void Unblinded::DiscardReservedUnblindedTokensFromCache(
    const std::string& contribution_id) {
  DCHECK(!contribution_id.empty());
  base::AutoLock lock(reserved_unblinded_tokens_lock_);
  reserved_unblinded_tokens_for_retry_.erase(contribution_id);
}

}  // namespace braveledger_contribution
