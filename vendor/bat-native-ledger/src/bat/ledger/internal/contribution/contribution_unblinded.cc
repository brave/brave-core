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
  if (!winners) {
    return false;
  }

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
  std::vector<ledger::ContributionPublisher> converted_list;

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

int32_t GetRetryCount(
    const ledger::ContributionStep step,
    ledger::ContributionInfoPtr contribution) {
  if (!contribution || step != contribution->step) {
    return 0;
  }

  return contribution->retry_count + 1;
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

void Unblinded::Initialize() {
  auto callback = std::bind(&Unblinded::OnGetNotCompletedContributions,
      this,
      _1);
  ledger_->GetIncompleteContributions(
      ledger::ContributionProcessor::BRAVE_TOKENS,
      callback);
}

void Unblinded::OnGetNotCompletedContributions(
    ledger::ContributionInfoList list) {
  if (list.size() == 0) {
    return;
  }

  if (!list[0]) {
    return;
  }

  DoRetry(std::move(list[0]));
}

void Unblinded::Start(const std::string& contribution_id) {
  GetContributionInfoAndUnblindedTokens(
      contribution_id,
      std::bind(&Unblinded::PrepareTokens,
          this,
          _1,
          _2));
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
    ContributionCompleted(ledger::Result::NOT_ENOUGH_FUNDS, nullptr);
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
    const std::vector<ledger::UnblindedToken>& list) {
  if (!contribution) {
    return;
  }

  const int32_t retry_count = GetRetryCount(
      ledger::ContributionStep::STEP_START,
      contribution->Clone());

  ledger_->UpdateContributionInfoStepAndCount(
      contribution->contribution_id,
      ledger::ContributionStep::STEP_START,
      retry_count,
      [](const ledger::Result){});

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
    ContributionCompleted(
        ledger::Result::NOT_ENOUGH_FUNDS,
        std::move(contribution));
    return;
  }

  PreparePublishers(token_list, std::move(contribution));
}

void Unblinded::PreparePublishers(
    const std::vector<ledger::UnblindedToken>& list,
    ledger::ContributionInfoPtr contribution) {
  if (!contribution) {
    return;
  }

  if (contribution->type == ledger::RewardsType::AUTO_CONTRIBUTE) {
    auto publisher_list =
        PrepareAutoContribution(list, contribution->Clone());

    if (publisher_list.empty()) {
      ContributionCompleted(
        ledger::Result::AC_TABLE_EMPTY,
        std::move(contribution));
      return;
    }

    contribution->publishers = std::move(publisher_list);

    ledger_->SaveContributionInfo(
      contribution->Clone(),
      std::bind(&Unblinded::OnPrepareAutoContribution,
          this,
          _1,
          contribution->contribution_id));
    return;
  }

  ProcessTokens(contribution->contribution_id);
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
    const std::string& contribution_id) {
  if (result != ledger::Result::LEDGER_OK) {
    SetTimer(contribution_id);
    return;
  }

  ProcessTokens(contribution_id);
}

void Unblinded::ProcessTokens(const std::string& contribution_id) {
  GetContributionInfoAndUnblindedTokens(
      contribution_id,
      std::bind(&Unblinded::OnProcessTokens,
          this,
          _1,
          _2));
}

void Unblinded::OnProcessTokens(
    ledger::ContributionInfoPtr contribution,
    const std::vector<ledger::UnblindedToken>& list) {
  if (!contribution) {
    return;
  }

  const int32_t retry_count = GetRetryCount(
      ledger::ContributionStep::STEP_SUGGESTIONS,
      contribution->Clone());

  ledger_->UpdateContributionInfoStepAndCount(
      contribution->contribution_id,
      ledger::ContributionStep::STEP_SUGGESTIONS,
      retry_count,
      [](const ledger::Result){});

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

    auto callback = std::bind(&Unblinded::TokenProcessed,
        this,
        _1,
        contribution->contribution_id,
        publisher->publisher_key);

    braveledger_credentials::CredentialsRedeem redeem;
    redeem.publisher_key = publisher->publisher_key;
    redeem.type = contribution->type;
    redeem.processor = contribution->processor;
    redeem.token_list = token_list;

    credentials_->RedeemTokens(redeem, callback);
    return;
  }

  ContributionCompleted(ledger::Result::LEDGER_OK, std::move(contribution));
}

void Unblinded::TokenProcessed(
    const ledger::Result result,
    const std::string& contribution_id,
    const std::string& publisher_key) {
  if (result == ledger::Result::LEDGER_OK) {
    auto callback = std::bind(&Unblinded::OnTokenProcessed,
        this,
        _1,
        contribution_id);

    ledger_->UpdateContributionInfoContributedAmount(
        contribution_id,
        publisher_key,
        callback);
    return;
  }

  SetTimer(contribution_id);
}

void Unblinded::OnTokenProcessed(
    const ledger::Result result,
    const std::string& contribution_id) {
  ledger_->GetContributionInfo(
      contribution_id,
      std::bind(&Unblinded::CheckIfCompleted,
                this,
                _1));
}

void Unblinded::CheckIfCompleted(ledger::ContributionInfoPtr contribution) {
  if (!contribution) {
    return;
  }

  bool completed = true;
  for (auto& publisher : contribution->publishers) {
    if (publisher->total_amount == publisher->contributed_amount) {
      continue;
    }

    completed = false;
    break;
  }

  if (completed) {
    ContributionCompleted(ledger::Result::LEDGER_OK, std::move(contribution));
    return;
  }

  SetTimer(contribution->contribution_id);
}

void Unblinded::ContributionCompleted(
    const ledger::Result result,
    ledger::ContributionInfoPtr contribution) {
  if (!contribution) {
    return;
  }

  ledger_->ContributionCompleted(
      result,
      contribution->amount,
      contribution->contribution_id,
      contribution->type);
}

void Unblinded::SetTimer(
    const std::string& contribution_id,
    const uint64_t& start_timer_in) {
  if (contribution_id.empty()) {
    return;
  }

  if (!retry_timers_[contribution_id]) {
    retry_timers_[contribution_id] = 0u;
  }

  uint64_t timer_seconds = start_timer_in;
  if (ledger::short_retries) {
    timer_seconds = 1;
  } else if (start_timer_in == 0) {
    timer_seconds = brave_base::random::Geometric(45);
  }

  BLOG(ledger_, ledger::LogLevel::LOG_INFO)
    << "Timer will start in "
    << timer_seconds;

  ledger_->SetTimer(timer_seconds, &retry_timers_[contribution_id]);
}

void Unblinded::OnTimer(uint32_t timer_id) {
  for (std::pair<std::string, uint32_t> const& value : retry_timers_) {
    if (value.second == timer_id) {
      std::string contribution_id = value.first;
      CheckStep(contribution_id);
      retry_timers_[contribution_id] = 0u;
    }
  }
}

void Unblinded::CheckStep(const std::string& contribution_id) {
  auto callback = std::bind(&Unblinded::DoRetry,
      this,
      _1);
  ledger_->GetContributionInfo(contribution_id, callback);
}

void Unblinded::DoRetry(ledger::ContributionInfoPtr contribution) {
  if (!contribution) {
    return;
  }

  if (contribution->step == ledger::ContributionStep::STEP_START) {
    Start(contribution->contribution_id);
    return;
  }

  if (contribution->step == ledger::ContributionStep::STEP_SUGGESTIONS) {
    ProcessTokens(contribution->contribution_id);
    return;
  }

  NOTREACHED();
}

}  // namespace braveledger_contribution
