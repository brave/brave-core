/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/contribution/contribution_sku.h"
#include "brave/components/brave_rewards/core/contribution/contribution_unblinded.h"
#include "brave/components/brave_rewards/core/contribution/contribution_util.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave_base/random.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace brave_rewards::internal::contribution {

namespace {

// Allocates one "vote" to a publisher. |dart| is a uniform random
// double in [0,1] "thrown" into the list of publishers to choose a
// winner. This function encapsulates the deterministic portion of
// choosing a winning publisher, separated out into a separate
// function for testing purposes.
std::string GetStatisticalVotingWinner(
    double dart,
    double amount,
    const std::vector<mojom::ContributionPublisherPtr>& publisher_list) {
  std::string publisher_key;

  double upper = 0.0;
  for (const auto& item : publisher_list) {
    upper += item->total_amount / amount;
    if (upper < dart) {
      continue;
    }

    publisher_key = item->publisher_key;
    break;
  }

  return publisher_key;
}

// Allocates "votes" to a list of publishers based on attention.
// |total_votes| is the number of votes to allocate (typically the
// number of unspent unblinded tokens). |publisher_list| is the list
// of publishers, sorted in ascending order by total_amount field.
void GetStatisticalVotingWinners(
    uint32_t total_votes,
    double amount,
    const std::vector<mojom::ContributionPublisherPtr>& publisher_list,
    StatisticalVotingWinners* winners) {
  DCHECK(winners);

  if (total_votes == 0 || publisher_list.empty()) {
    return;
  }

  // Initialize all potential winners to 0, as it's possible that one
  // or more publishers may receive no votes at all
  for (const auto& item : publisher_list) {
    winners->emplace(item->publisher_key, 0);
  }

  while (total_votes > 0) {
    const double dart = brave_base::random::Uniform_01();
    const std::string publisher_key =
        GetStatisticalVotingWinner(dart, amount, publisher_list);
    if (publisher_key.empty()) {
      continue;
    }

    auto iter = winners->find(publisher_key);
    if (iter != winners->end()) {
      const uint32_t current_value = winners->at(publisher_key);
      winners->at(publisher_key) = current_value + 1;
    } else {
      winners->emplace(publisher_key, 1);
    }

    --total_votes;
  }
}

}  // namespace

Unblinded::Unblinded(RewardsEngineImpl& engine)
    : engine_(engine),
      credentials_promotion_(engine),
      credentials_sku_(engine) {}

Unblinded::~Unblinded() = default;

void Unblinded::Start(const std::vector<mojom::CredsBatchType>& types,
                      const std::string& contribution_id,
                      LegacyResultCallback callback) {
  if (contribution_id.empty()) {
    engine_->LogError(FROM_HERE) << "Contribution id is empty";
    callback(mojom::Result::FAILED);
    return;
  }

  auto get_callback =
      std::bind(&Unblinded::PrepareTokens, this, _1, _2, types, callback);

  GetContributionInfoAndUnblindedTokens(types, contribution_id, get_callback);
}

void Unblinded::GetContributionInfoAndUnblindedTokens(
    const std::vector<mojom::CredsBatchType>& types,
    const std::string& contribution_id,
    GetContributionInfoAndUnblindedTokensCallback callback) {
  auto get_callback = std::bind(&Unblinded::OnUnblindedTokens, this, _1,
                                contribution_id, callback);
  engine_->database()->GetSpendableUnblindedTokensByBatchTypes(types,
                                                               get_callback);
}

void Unblinded::OnUnblindedTokens(
    std::vector<mojom::UnblindedTokenPtr> unblinded_tokens,
    const std::string& contribution_id,
    GetContributionInfoAndUnblindedTokensCallback callback) {
  if (unblinded_tokens.empty()) {
    engine_->Log(FROM_HERE) << "Token list is empty";
  }

  std::vector<mojom::UnblindedToken> converted_list;
  for (const auto& item : unblinded_tokens) {
    mojom::UnblindedToken new_item;
    new_item.id = item->id;
    new_item.token_value = item->token_value;
    new_item.public_key = item->public_key;
    new_item.value = item->value;
    new_item.creds_id = item->creds_id;
    new_item.expires_at = item->expires_at;

    converted_list.push_back(new_item);
  }

  engine_->database()->GetContributionInfo(
      contribution_id, std::bind(&Unblinded::OnGetContributionInfo, this, _1,
                                 std::move(converted_list), callback));
}

void Unblinded::GetContributionInfoAndReservedUnblindedTokens(
    const std::string& contribution_id,
    GetContributionInfoAndUnblindedTokensCallback callback) {
  auto get_callback = std::bind(&Unblinded::OnReservedUnblindedTokens, this, _1,
                                contribution_id, callback);
  engine_->database()->GetReservedUnblindedTokens(contribution_id,
                                                  get_callback);
}

void Unblinded::OnReservedUnblindedTokens(
    std::vector<mojom::UnblindedTokenPtr> unblinded_tokens,
    const std::string& contribution_id,
    GetContributionInfoAndUnblindedTokensCallback callback) {
  if (unblinded_tokens.empty()) {
    engine_->Log(FROM_HERE) << "Token list is empty";
  }

  std::vector<mojom::UnblindedToken> converted_list;
  for (const auto& item : unblinded_tokens) {
    mojom::UnblindedToken new_item;
    new_item.id = item->id;
    new_item.token_value = item->token_value;
    new_item.public_key = item->public_key;
    new_item.value = item->value;
    new_item.creds_id = item->creds_id;
    new_item.expires_at = item->expires_at;

    converted_list.push_back(new_item);
  }

  engine_->database()->GetContributionInfo(
      contribution_id, std::bind(&Unblinded::OnGetContributionInfo, this, _1,
                                 converted_list, callback));
}

void Unblinded::OnGetContributionInfo(
    mojom::ContributionInfoPtr contribution,
    const std::vector<mojom::UnblindedToken>& unblinded_tokens,
    GetContributionInfoAndUnblindedTokensCallback callback) {
  callback(std::move(contribution), unblinded_tokens);
}

void Unblinded::PrepareTokens(
    mojom::ContributionInfoPtr contribution,
    const std::vector<mojom::UnblindedToken>& unblinded_tokens,
    const std::vector<mojom::CredsBatchType>& types,
    LegacyResultCallback callback) {
  if (!contribution) {
    engine_->LogError(FROM_HERE) << "Contribution not found";
    callback(mojom::Result::FAILED);
    return;
  }

  if (unblinded_tokens.empty()) {
    engine_->LogError(FROM_HERE) << "Not enough funds";
    callback(mojom::Result::NOT_ENOUGH_FUNDS);
    return;
  }

  double current_amount = 0.0;
  std::vector<mojom::UnblindedToken> token_list;
  for (const auto& item : unblinded_tokens) {
    if (current_amount >= contribution->amount) {
      break;
    }

    current_amount += item.value;
    token_list.push_back(item);
  }

  if (current_amount < contribution->amount) {
    callback(mojom::Result::NOT_ENOUGH_FUNDS);
    engine_->LogError(FROM_HERE) << "Not enough funds";
    return;
  }

  const std::string contribution_id = contribution->contribution_id;

  std::vector<std::string> token_id_list;
  for (const auto& item : token_list) {
    token_id_list.push_back(base::NumberToString(item.id));
  }

  auto reserved_callback = std::bind(
      &Unblinded::OnMarkUnblindedTokensAsReserved, this, _1,
      std::move(token_list),
      std::make_shared<mojom::ContributionInfoPtr>(contribution->Clone()),
      types, callback);

  engine_->database()->MarkUnblindedTokensAsReserved(
      token_id_list, contribution_id, reserved_callback);
}

void Unblinded::OnMarkUnblindedTokensAsReserved(
    mojom::Result result,
    const std::vector<mojom::UnblindedToken>& unblinded_tokens,
    std::shared_ptr<mojom::ContributionInfoPtr> shared_contribution,
    const std::vector<mojom::CredsBatchType>& types,
    LegacyResultCallback callback) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Failed to reserve unblinded tokens";
    callback(mojom::Result::FAILED);
    return;
  }

  if (!shared_contribution) {
    engine_->LogError(FROM_HERE)
        << "Contribution was not converted successfully";
    callback(mojom::Result::FAILED);
    return;
  }

  PreparePublishers(unblinded_tokens, std::move(*shared_contribution), types,
                    callback);
}

void Unblinded::PreparePublishers(
    const std::vector<mojom::UnblindedToken>& unblinded_tokens,
    mojom::ContributionInfoPtr contribution,
    const std::vector<mojom::CredsBatchType>& types,
    LegacyResultCallback callback) {
  if (!contribution) {
    engine_->LogError(FROM_HERE) << "Contribution not found";
    callback(mojom::Result::FAILED);
    return;
  }

  if (contribution->type == mojom::RewardsType::AUTO_CONTRIBUTE) {
    auto publisher_list =
        PrepareAutoContribution(unblinded_tokens, contribution->Clone());

    if (publisher_list.empty()) {
      engine_->LogError(FROM_HERE) << "Publisher list empty";
      callback(mojom::Result::AC_TABLE_EMPTY);
      return;
    }

    contribution->publishers = std::move(publisher_list);

    auto save_callback =
        std::bind(&Unblinded::OnPrepareAutoContribution, this, _1, types,
                  contribution->contribution_id, callback);

    engine_->database()->SaveContributionInfo(contribution->Clone(),
                                              save_callback);
    return;
  }

  auto save_callback = std::bind(&Unblinded::PrepareStepSaved, this, _1, types,
                                 contribution->contribution_id, callback);

  engine_->database()->UpdateContributionInfoStep(
      contribution->contribution_id, mojom::ContributionStep::STEP_PREPARE,
      save_callback);
}

std::vector<mojom::ContributionPublisherPtr> Unblinded::PrepareAutoContribution(
    const std::vector<mojom::UnblindedToken>& unblinded_tokens,
    mojom::ContributionInfoPtr contribution) {
  if (!contribution) {
    engine_->LogError(FROM_HERE) << "Contribution is null";
    return {};
  }

  if (unblinded_tokens.size() == 0) {
    engine_->LogError(FROM_HERE) << "Token list is empty";
    return {};
  }

  if (contribution->publishers.empty()) {
    engine_->LogError(FROM_HERE) << "Publisher list is empty";
    return {};
  }

  const double total_votes = static_cast<double>(unblinded_tokens.size());
  StatisticalVotingWinners winners;
  GetStatisticalVotingWinners(total_votes, contribution->amount,
                              std::move(contribution->publishers), &winners);

  std::vector<mojom::ContributionPublisherPtr> publisher_list;
  for (const auto& winner : winners) {
    const std::string publisher_key = winner.first;
    auto publisher = mojom::ContributionPublisher::New();
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
    mojom::Result result,
    const std::vector<mojom::CredsBatchType>& types,
    const std::string& contribution_id,
    LegacyResultCallback callback) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Contribution not saved";
    callback(mojom::Result::RETRY);
    return;
  }

  auto save_callback = std::bind(&Unblinded::PrepareStepSaved, this, _1, types,
                                 contribution_id, callback);

  engine_->database()->UpdateContributionInfoStep(
      contribution_id, mojom::ContributionStep::STEP_PREPARE, save_callback);
}

void Unblinded::PrepareStepSaved(
    mojom::Result result,
    const std::vector<mojom::CredsBatchType>& types,
    const std::string& contribution_id,
    LegacyResultCallback callback) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Prepare step was not saved";
    callback(mojom::Result::RETRY);
    return;
  }

  ProcessTokens(types, contribution_id, callback);
}

void Unblinded::ProcessTokens(const std::vector<mojom::CredsBatchType>& types,
                              const std::string& contribution_id,
                              LegacyResultCallback callback) {
  auto get_callback =
      std::bind(&Unblinded::OnProcessTokens, this, _1, _2, callback);
  GetContributionInfoAndReservedUnblindedTokens(contribution_id, get_callback);
}

void Unblinded::OnProcessTokens(
    mojom::ContributionInfoPtr contribution,
    const std::vector<mojom::UnblindedToken>& unblinded_tokens,
    LegacyResultCallback callback) {
  if (!contribution || contribution->publishers.empty()) {
    engine_->LogError(FROM_HERE) << "Contribution not found";
    callback(mojom::Result::FAILED);
    return;
  }

  bool final_publisher = false;
  for (auto publisher = contribution->publishers.begin();
       publisher != contribution->publishers.end(); publisher++) {
    if ((*publisher)->total_amount == (*publisher)->contributed_amount) {
      continue;
    }

    if (std::next(publisher) == contribution->publishers.end()) {
      final_publisher = true;
    }

    std::vector<mojom::UnblindedToken> token_list;
    double current_amount = 0.0;
    for (auto& item : unblinded_tokens) {
      if (current_amount >= (*publisher)->total_amount) {
        break;
      }

      current_amount += item.value;
      token_list.push_back(item);
    }

    auto redeem_callback = std::bind(
        &Unblinded::TokenProcessed, this, _1, contribution->contribution_id,
        (*publisher)->publisher_key, final_publisher, callback);

    credential::CredentialsRedeem redeem;
    redeem.publisher_key = (*publisher)->publisher_key;
    redeem.type = contribution->type;
    redeem.processor = contribution->processor;
    redeem.token_list = token_list;
    redeem.contribution_id = contribution->contribution_id;

    if (redeem.processor == mojom::ContributionProcessor::UPHOLD ||
        redeem.processor == mojom::ContributionProcessor::GEMINI) {
      credentials_sku_.RedeemTokens(redeem, redeem_callback);
      return;
    }

    credentials_promotion_.RedeemTokens(redeem, redeem_callback);
    return;
  }

  // we processed all publishers
  callback(mojom::Result::OK);
}

void Unblinded::TokenProcessed(mojom::Result result,
                               const std::string& contribution_id,
                               const std::string& publisher_key,
                               bool final_publisher,
                               LegacyResultCallback callback) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Tokens were not processed correctly";
    callback(mojom::Result::RETRY);
    return;
  }

  auto save_callback = std::bind(&Unblinded::ContributionAmountSaved, this, _1,
                                 contribution_id, final_publisher, callback);

  engine_->database()->UpdateContributionInfoContributedAmount(
      contribution_id, publisher_key, save_callback);
}

void Unblinded::ContributionAmountSaved(mojom::Result result,
                                        const std::string& contribution_id,
                                        bool final_publisher,
                                        LegacyResultCallback callback) {
  if (final_publisher) {
    callback(result);
    return;
  }

  callback(mojom::Result::RETRY_LONG);
}

void Unblinded::Retry(const std::vector<mojom::CredsBatchType>& types,
                      mojom::ContributionInfoPtr contribution,
                      LegacyResultCallback callback) {
  if (!contribution) {
    engine_->LogError(FROM_HERE) << "Contribution is null";
    callback(mojom::Result::FAILED);
    return;
  }

  const bool is_not_tokens =
      contribution->processor != mojom::ContributionProcessor::BRAVE_TOKENS;

  const bool is_not_uphold_ac =
      contribution->processor == mojom::ContributionProcessor::UPHOLD &&
      contribution->type != mojom::RewardsType::AUTO_CONTRIBUTE;

  if (is_not_tokens && is_not_uphold_ac) {
    engine_->LogError(FROM_HERE) << "Retry is not for this func";
    callback(mojom::Result::FAILED);
    return;
  }

  switch (contribution->step) {
    case mojom::ContributionStep::STEP_START: {
      Start(types, contribution->contribution_id, callback);
      return;
    }
    case mojom::ContributionStep::STEP_PREPARE: {
      ProcessTokens(types, contribution->contribution_id, callback);
      return;
    }
    case mojom::ContributionStep::STEP_RESERVE: {
      auto get_callback = std::bind(
          &Unblinded::OnReservedUnblindedTokensForRetryAttempt, this, _1, types,
          std::make_shared<mojom::ContributionInfoPtr>(contribution->Clone()),
          callback);
      engine_->database()->GetReservedUnblindedTokens(
          contribution->contribution_id, get_callback);
      return;
    }
    case mojom::ContributionStep::STEP_RETRY_COUNT:
    case mojom::ContributionStep::STEP_REWARDS_OFF:
    case mojom::ContributionStep::STEP_AC_OFF:
    case mojom::ContributionStep::STEP_AC_TABLE_EMPTY:
    case mojom::ContributionStep::STEP_CREDS:
    case mojom::ContributionStep::STEP_EXTERNAL_TRANSACTION:
    case mojom::ContributionStep::STEP_NOT_ENOUGH_FUNDS:
    case mojom::ContributionStep::STEP_FAILED:
    case mojom::ContributionStep::STEP_COMPLETED:
    case mojom::ContributionStep::STEP_NO: {
      engine_->LogError(FROM_HERE) << "Step not correct " << contribution->step;
      NOTREACHED();
      return;
    }
  }
}

void Unblinded::OnReservedUnblindedTokensForRetryAttempt(
    const std::vector<mojom::UnblindedTokenPtr>& unblinded_tokens,
    const std::vector<mojom::CredsBatchType>& types,
    std::shared_ptr<mojom::ContributionInfoPtr> shared_contribution,
    LegacyResultCallback callback) {
  if (unblinded_tokens.empty()) {
    engine_->LogError(FROM_HERE) << "Token list is empty";
    callback(mojom::Result::FAILED);
    return;
  }

  if (!shared_contribution) {
    engine_->LogError(FROM_HERE)
        << "Contribution was not converted successfully";
    callback(mojom::Result::FAILED);
    return;
  }

  std::vector<mojom::UnblindedToken> converted_list;
  for (const auto& item : unblinded_tokens) {
    mojom::UnblindedToken new_item;
    new_item.id = item->id;
    new_item.token_value = item->token_value;
    new_item.public_key = item->public_key;
    new_item.value = item->value;
    new_item.creds_id = item->creds_id;
    new_item.expires_at = item->expires_at;

    converted_list.push_back(new_item);
  }

  PreparePublishers(converted_list, std::move(*shared_contribution), types,
                    callback);
}

std::string Unblinded::GetStatisticalVotingWinnerForTesting(
    double dart,
    double amount,
    const std::vector<mojom::ContributionPublisherPtr>& publisher_list) {
  return GetStatisticalVotingWinner(dart, amount, publisher_list);
}

}  // namespace brave_rewards::internal::contribution
