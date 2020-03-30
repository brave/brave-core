/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ledger/internal/bat_util.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/contribution/contribution_unblinded.h"
#include "bat/ledger/internal/contribution/contribution_sku.h"
#include "bat/ledger/internal/contribution/contribution_util.h"
#include "bat/ledger/internal/request/request_promotion.h"
#include "brave_base/random.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace {

bool HasTokenExpired(const ledger::UnblindedToken& token) {
  const auto now = braveledger_time_util::GetCurrentTimeStamp();

  return token.expires_at > 0 && token.expires_at < now;
}

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
  credentials_ = braveledger_credentials::CredentialsFactory::Create(
      ledger_,
      ledger::CredsBatchType::PROMOTION);
  DCHECK(credentials_);
}

Unblinded::~Unblinded() = default;

void Unblinded::Start(
    const std::string& contribution_id,
    ledger::ResultCallback callback) {
  if (contribution_id.empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Contribution id is empty";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  GetContributionInfoAndUnblindedTokens(
      contribution_id,
      std::bind(&Unblinded::PrepareTokens,
          this,
          _1,
          _2,
          callback));
}

void Unblinded::GetContributionInfoAndUnblindedTokens(
    const std::string& contribution_id,
    GetContributionInfoAndUnblindedTokensCallback callback) {
  ledger_->GetAllUnblindedTokens(
    std::bind(&Unblinded::OnUnblindedTokens,
        this,
        _1,
        contribution_id,
        callback));
}

void Unblinded::OnUnblindedTokens(
    ledger::UnblindedTokenList list,
    const std::string& contribution_id,
    GetContributionInfoAndUnblindedTokensCallback callback) {
  if (list.empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Token list is empty";
    callback(nullptr, {});
    return;
  }

  std::vector<ledger::UnblindedToken> converted_list;
  for (auto& item : list) {
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
    ledger::ResultCallback callback) {
  if (!contribution) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Contribution not found";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (list.empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Not enough funds";
    callback(ledger::Result::NOT_ENOUGH_FUNDS);
    return;
  }

  double current_amount = 0.0;
  std::vector<ledger::UnblindedToken> token_list;
  std::vector<std::string> delete_list;
  for (auto & item : list) {
    if (HasTokenExpired(item)) {
      delete_list.push_back(std::to_string(item.id));
      continue;
    }

    if (current_amount >= contribution->amount) {
      break;
    }

    current_amount += item.value;
    token_list.push_back(item);
  }

  if (delete_list.size() > 0) {
    ledger_->DeleteUnblindedTokens(delete_list, [](const ledger::Result _){});
  }

  if (current_amount < contribution->amount) {
    callback(ledger::Result::NOT_ENOUGH_FUNDS);
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Not enough funds";
    return;
  }

  // TODO(https://github.com/brave/brave-browser/issues/8887):
  // we should reserve this tokens and add step STEP_RESERVE
  PreparePublishers(token_list, std::move(contribution), callback);
}

void Unblinded::PreparePublishers(
    const std::vector<ledger::UnblindedToken>& list,
    ledger::ContributionInfoPtr contribution,
    ledger::ResultCallback callback) {
  if (!contribution) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Contribution not found";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (contribution->type == ledger::RewardsType::AUTO_CONTRIBUTE) {
    auto publisher_list =
        PrepareAutoContribution(list, contribution->Clone());

    if (publisher_list.empty()) {
      BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Publisher list empty";
      callback(ledger::Result::AC_TABLE_EMPTY);
      return;
    }

    contribution->publishers = std::move(publisher_list);

    auto save_callback = std::bind(&Unblinded::OnPrepareAutoContribution,
        this,
        _1,
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
  if (!contribution || list.size() == 0 || contribution->publishers.empty()) {
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
    const std::string& contribution_id,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Contribution not saved";
    callback(ledger::Result::RETRY);
    return;
  }

  auto save_callback = std::bind(&Unblinded::PrepareStepSaved,
      this,
      _1,
      contribution_id,
      callback);

  ledger_->UpdateContributionInfoStep(
      contribution_id,
      ledger::ContributionStep::STEP_PREPARE,
      save_callback);
}

void Unblinded::PrepareStepSaved(
    const ledger::Result result,
    const std::string& contribution_id,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Prepare step was not saved";
    callback(ledger::Result::RETRY);
    return;
  }

  ProcessTokens(contribution_id, callback);
}

void Unblinded::ProcessTokens(
    const std::string& contribution_id,
    ledger::ResultCallback callback) {
  // TODO(https://github.com/brave/brave-browser/issues/8887):
  // here we should fetch reserved tokens so that in OnProcessTokens
  // no additional computing is needed
  GetContributionInfoAndUnblindedTokens(
      contribution_id,
      std::bind(&Unblinded::OnProcessTokens,
          this,
          _1,
          _2,
          callback));
}

void Unblinded::OnProcessTokens(
    ledger::ContributionInfoPtr contribution,
    const std::vector<ledger::UnblindedToken>& list,
    ledger::ResultCallback callback) {
  if (!contribution || contribution->publishers.empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Contribution not found";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  bool single_publisher = contribution->publishers.size() == 1;

  for (auto& publisher : contribution->publishers) {
    if (publisher->total_amount == publisher->contributed_amount) {
      continue;
    }

    std::vector<ledger::UnblindedToken> token_list;
    double current_amount = 0.0;
    for (auto& item : list) {
      if (current_amount >= publisher->total_amount) {
        break;
      }

      current_amount += item.value;
      token_list.push_back(item);
    }

    auto redeem_callback = std::bind(&Unblinded::TokenProcessed,
        this,
        _1,
        contribution->contribution_id,
        publisher->publisher_key,
        single_publisher,
        callback);

    braveledger_credentials::CredentialsRedeem redeem;
    redeem.publisher_key = publisher->publisher_key;
    redeem.type = contribution->type;
    redeem.processor = contribution->processor;
    redeem.token_list = token_list;

    credentials_->RedeemTokens(redeem, redeem_callback);
    return;
  }

  // we processed all publishers
  callback(ledger::Result::LEDGER_OK);
}

void Unblinded::TokenProcessed(
    const ledger::Result result,
    const std::string& contribution_id,
    const std::string& publisher_key,
    const bool single_publisher,
    ledger::ResultCallback callback) {
  if (result == ledger::Result::LEDGER_OK) {
    auto save_callback = std::bind(&Unblinded::ContributionAmountSaved,
        this,
        _1,
        contribution_id,
        single_publisher,
        callback);

    ledger_->UpdateContributionInfoContributedAmount(
        contribution_id,
        publisher_key,
        save_callback);
    return;
  }

  BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
      << "Tokens were not processed correctly";
  callback(ledger::Result::RETRY);
}

void Unblinded::ContributionAmountSaved(
    const ledger::Result result,
    const std::string& contribution_id,
    const bool single_publisher,
    ledger::ResultCallback callback) {
  if (single_publisher) {
    callback(result);
    return;
  }

  callback(ledger::Result::RETRY);
}

void Unblinded::Retry(
    ledger::ContributionInfoPtr contribution,
    ledger::ResultCallback callback) {
  if (!contribution) {
    return;
  }

  const bool is_not_tokens =
      contribution->processor != ledger::ContributionProcessor::BRAVE_TOKENS;

  const bool is_not_uphold_ac =
      contribution->processor == ledger::ContributionProcessor::UPHOLD &&
      contribution->type != ledger::RewardsType::AUTO_CONTRIBUTE;

  if (is_not_tokens && is_not_uphold_ac) {
    return;
  }

  if (contribution->step == ledger::ContributionStep::STEP_START) {
    Start(contribution->contribution_id, callback);
    return;
  }

  if (contribution->step == ledger::ContributionStep::STEP_PREPARE) {
    ProcessTokens(contribution->contribution_id, callback);
    return;
  }

  NOTREACHED();
}

}  // namespace braveledger_contribution
