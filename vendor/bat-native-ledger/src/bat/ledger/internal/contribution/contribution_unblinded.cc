/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/base64.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ledger/internal/bat_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/contribution/contribution_unblinded.h"
#include "bat/ledger/internal/request/promotion_requests.h"
#include "brave_base/random.h"
#include "net/http/http_status_code.h"

#include "wrapper.hpp"  // NOLINT

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

using challenge_bypass_ristretto::UnblindedToken;
using challenge_bypass_ristretto::VerificationKey;
using challenge_bypass_ristretto::VerificationSignature;

namespace {

std::string ConvertTypeToString(const ledger::RewardsType type) {
  switch (static_cast<int>(type)) {
    case static_cast<int>(ledger::RewardsType::AUTO_CONTRIBUTE): {
      return "auto-contribute";
    }
    case static_cast<int>(ledger::RewardsType::ONE_TIME_TIP): {
      return "oneoff-tip";
    }
    case static_cast<int>(ledger::RewardsType::RECURRING_TIP): {
      return "recurring-tip";
    }
    default: {
      // missing conversion, returning dummy value.
      NOTREACHED();
      return "oneoff-tip";
    }
  }
}

void GenerateSuggestionMock(
    const ledger::UnblindedToken& token,
    const std::string& suggestion_encoded,
    base::Value* result) {
  result->SetStringKey("t", token.token_value);
  result->SetStringKey("publicKey", token.public_key);
  result->SetStringKey("signature", token.token_value);
}

void GenerateSuggestion(
    const ledger::UnblindedToken& token,
    const std::string& suggestion_encoded,
    base::Value* result) {
  UnblindedToken unblinded = UnblindedToken::decode_base64(token.token_value);
  VerificationKey verification_key = unblinded.derive_verification_key();
  VerificationSignature signature = verification_key.sign(suggestion_encoded);
  const std::string pre_image = unblinded.preimage().encode_base64();

  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    return;
  }

  result->SetStringKey("t", pre_image);
  result->SetStringKey("publicKey", token.public_key);
  result->SetStringKey("signature", signature.encode_base64());
}

bool HasTokenExpired(const ledger::UnblindedToken& token) {
  const uint64_t now = static_cast<uint64_t>(base::Time::Now().ToDoubleT());

  return token.expires_at > 0 && token.expires_at < now;
}

std::string GenerateTokenPayload(
    const std::string& publisher_key,
    const ledger::RewardsType type,
    const std::vector<ledger::UnblindedToken>& list) {
  base::Value suggestion(base::Value::Type::DICTIONARY);
  suggestion.SetStringKey("type", ConvertTypeToString(type));
  suggestion.SetStringKey("channel", publisher_key);

  std::string suggestion_json;
  base::JSONWriter::Write(suggestion, &suggestion_json);
  std::string suggestion_encoded;
  base::Base64Encode(suggestion_json, &suggestion_encoded);

  base::Value credentials(base::Value::Type::LIST);
  for (auto & item : list) {
    base::Value token(base::Value::Type::DICTIONARY);
    if (ledger::is_testing) {
      GenerateSuggestionMock(item, suggestion_encoded, &token);
    } else {
      GenerateSuggestion(item, suggestion_encoded, &token);
    }
    credentials.GetList().push_back(std::move(token));
  }

  base::Value payload(base::Value::Type::DICTIONARY);
  payload.SetStringKey("suggestion", suggestion_encoded);
  payload.SetKey("credentials", std::move(credentials));

  std::string json;
  base::JSONWriter::Write(payload, &json);
  return json;
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
}

Unblinded::~Unblinded() = default;

void Unblinded::Initialize() {
  auto callback = std::bind(&Unblinded::OnGetNotCompletedContributions,
      this,
      _1);
  ledger_->GetIncompleteContributions(callback);
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
    new_item.promotion_id = item->promotion_id;
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
  if (!contribution || list.size() == 0) {
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

    SendTokens(
        publisher->publisher_key,
        contribution->type,
        token_list,
        callback);
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

void Unblinded::SendTokens(
    const std::string& publisher_key,
    const ledger::RewardsType type,
    const std::vector<ledger::UnblindedToken>& list,
    ledger::ResultCallback callback) {
  if (publisher_key.empty() || list.empty()) {
    return callback(ledger::Result::LEDGER_ERROR);
  }

  std::vector<std::string> token_id_list;
  for (auto & item : list) {
    token_id_list.push_back(std::to_string(item.id));
  }

  auto url_callback = std::bind(&Unblinded::OnSendTokens,
      this,
      _1,
      _2,
      _3,
      token_id_list,
      callback);

  const std::string payload = GenerateTokenPayload(
      publisher_key,
      type,
      list);

  const std::string url =
      braveledger_request_util::GetReedemSuggestionsUrl();

  ledger_->LoadURL(
      url,
      std::vector<std::string>(),
      payload,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void Unblinded::OnSendTokens(
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const std::vector<std::string>& token_id_list,
    ledger::ResultCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  ledger_->DeleteUnblindedTokens(token_id_list, [](const ledger::Result _){});
  callback(ledger::Result::LEDGER_OK);
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
