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
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave_base/random.h"

namespace brave_rewards::internal::contribution {

namespace {

using StatisticalVotingWinners = std::map<std::string, uint32_t>;

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

Unblinded::Unblinded(RewardsEngine& engine)
    : engine_(engine), credentials_sku_(engine) {}

Unblinded::~Unblinded() = default;

void Unblinded::Start(const std::vector<mojom::CredsBatchType>& types,
                      const std::string& contribution_id,
                      ResultCallback callback) {
  if (contribution_id.empty()) {
    engine_->LogError(FROM_HERE) << "Contribution id is empty";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  GetContributionInfoAndUnblindedTokens(
      types, contribution_id,
      base::BindOnce(&Unblinded::PrepareTokens, weak_factory_.GetWeakPtr(),
                     types, std::move(callback)));
}

void Unblinded::GetContributionInfoAndUnblindedTokens(
    const std::vector<mojom::CredsBatchType>& types,
    const std::string& contribution_id,
    GetContributionInfoAndUnblindedTokensCallback callback) {
  engine_->database()->GetSpendableUnblindedTokensByBatchTypes(
      types,
      base::BindOnce(&Unblinded::OnUnblindedTokens, weak_factory_.GetWeakPtr(),
                     contribution_id, std::move(callback)));
}

void Unblinded::OnUnblindedTokens(
    const std::string& contribution_id,
    GetContributionInfoAndUnblindedTokensCallback callback,
    std::vector<mojom::UnblindedTokenPtr> unblinded_tokens) {
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
      contribution_id,
      base::BindOnce(&Unblinded::OnGetContributionInfo,
                     weak_factory_.GetWeakPtr(), std::move(converted_list),
                     std::move(callback)));
}

void Unblinded::GetContributionInfoAndReservedUnblindedTokens(
    const std::string& contribution_id,
    GetContributionInfoAndUnblindedTokensCallback callback) {
  engine_->database()->GetReservedUnblindedTokens(
      contribution_id, base::BindOnce(&Unblinded::OnReservedUnblindedTokens,
                                      weak_factory_.GetWeakPtr(),
                                      contribution_id, std::move(callback)));
}

void Unblinded::OnReservedUnblindedTokens(
    const std::string& contribution_id,
    GetContributionInfoAndUnblindedTokensCallback callback,
    std::vector<mojom::UnblindedTokenPtr> unblinded_tokens) {
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
      contribution_id,
      base::BindOnce(&Unblinded::OnGetContributionInfo,
                     weak_factory_.GetWeakPtr(), std::move(converted_list),
                     std::move(callback)));
}

void Unblinded::OnGetContributionInfo(
    std::vector<mojom::UnblindedToken> unblinded_tokens,
    GetContributionInfoAndUnblindedTokensCallback callback,
    mojom::ContributionInfoPtr contribution) {
  std::move(callback).Run(std::move(contribution), unblinded_tokens);
}

void Unblinded::PrepareTokens(
    std::vector<mojom::CredsBatchType> types,
    ResultCallback callback,
    mojom::ContributionInfoPtr contribution,
    std::vector<mojom::UnblindedToken> unblinded_tokens) {
  if (!contribution) {
    engine_->LogError(FROM_HERE) << "Contribution not found";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  if (unblinded_tokens.empty()) {
    engine_->LogError(FROM_HERE) << "Not enough funds";
    std::move(callback).Run(mojom::Result::NOT_ENOUGH_FUNDS);
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
    std::move(callback).Run(mojom::Result::NOT_ENOUGH_FUNDS);
    engine_->LogError(FROM_HERE) << "Not enough funds";
    return;
  }

  const std::string contribution_id = contribution->contribution_id;

  std::vector<std::string> token_id_list;
  for (const auto& item : token_list) {
    token_id_list.push_back(base::NumberToString(item.id));
  }

  engine_->database()->MarkUnblindedTokensAsReserved(
      token_id_list, contribution_id,
      base::BindOnce(&Unblinded::OnMarkUnblindedTokensAsReserved,
                     weak_factory_.GetWeakPtr(), std::move(token_list),
                     std::move(contribution), std::move(types),
                     std::move(callback)));
}

void Unblinded::OnMarkUnblindedTokensAsReserved(
    std::vector<mojom::UnblindedToken> unblinded_tokens,
    mojom::ContributionInfoPtr contribution,
    std::vector<mojom::CredsBatchType> types,
    ResultCallback callback,
    mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Failed to reserve unblinded tokens";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  if (!contribution) {
    engine_->LogError(FROM_HERE)
        << "Contribution was not converted successfully";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  PreparePublishers(unblinded_tokens, std::move(contribution), types,
                    std::move(callback));
}

void Unblinded::PreparePublishers(
    const std::vector<mojom::UnblindedToken>& unblinded_tokens,
    mojom::ContributionInfoPtr contribution,
    const std::vector<mojom::CredsBatchType>& types,
    ResultCallback callback) {
  if (!contribution) {
    engine_->LogError(FROM_HERE) << "Contribution not found";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  if (contribution->type == mojom::RewardsType::AUTO_CONTRIBUTE) {
    auto publisher_list =
        PrepareAutoContribution(unblinded_tokens, contribution->Clone());

    if (publisher_list.empty()) {
      engine_->LogError(FROM_HERE) << "Publisher list empty";
      std::move(callback).Run(mojom::Result::AC_TABLE_EMPTY);
      return;
    }

    contribution->publishers = std::move(publisher_list);

    std::string contribution_id = contribution->contribution_id;

    engine_->database()->SaveContributionInfo(
        std::move(contribution),
        base::BindOnce(&Unblinded::OnPrepareAutoContribution,
                       weak_factory_.GetWeakPtr(), std::move(types),
                       contribution_id, std::move(callback)));
    return;
  }

  engine_->database()->UpdateContributionInfoStep(
      contribution->contribution_id, mojom::ContributionStep::STEP_PREPARE,
      base::BindOnce(&Unblinded::PrepareStepSaved, weak_factory_.GetWeakPtr(),
                     std::move(types), contribution->contribution_id,
                     std::move(callback)));
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
    std::vector<mojom::CredsBatchType> types,
    const std::string& contribution_id,
    ResultCallback callback,
    mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Contribution not saved";
    std::move(callback).Run(mojom::Result::RETRY);
    return;
  }

  engine_->database()->UpdateContributionInfoStep(
      contribution_id, mojom::ContributionStep::STEP_PREPARE,
      base::BindOnce(&Unblinded::PrepareStepSaved, weak_factory_.GetWeakPtr(),
                     std::move(types), contribution_id, std::move(callback)));
}

void Unblinded::PrepareStepSaved(std::vector<mojom::CredsBatchType> types,
                                 const std::string& contribution_id,
                                 ResultCallback callback,
                                 mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Prepare step was not saved";
    std::move(callback).Run(mojom::Result::RETRY);
    return;
  }

  ProcessTokens(types, contribution_id, std::move(callback));
}

void Unblinded::ProcessTokens(const std::vector<mojom::CredsBatchType>& types,
                              const std::string& contribution_id,
                              ResultCallback callback) {
  GetContributionInfoAndReservedUnblindedTokens(
      contribution_id,
      base::BindOnce(&Unblinded::OnProcessTokens, weak_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void Unblinded::OnProcessTokens(
    ResultCallback callback,
    mojom::ContributionInfoPtr contribution,
    std::vector<mojom::UnblindedToken> unblinded_tokens) {
  if (!contribution || contribution->publishers.empty()) {
    engine_->LogError(FROM_HERE) << "Contribution not found";
    std::move(callback).Run(mojom::Result::FAILED);
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

    auto redeem_callback = base::BindOnce(
        &Unblinded::TokenProcessed, weak_factory_.GetWeakPtr(),
        contribution->contribution_id, (*publisher)->publisher_key,
        final_publisher, std::move(callback));

    credential::CredentialsRedeem redeem;
    redeem.publisher_key = (*publisher)->publisher_key;
    redeem.type = contribution->type;
    redeem.processor = contribution->processor;
    redeem.token_list = token_list;
    redeem.contribution_id = contribution->contribution_id;

    if (redeem.processor == mojom::ContributionProcessor::UPHOLD ||
        redeem.processor == mojom::ContributionProcessor::GEMINI) {
      credentials_sku_.RedeemTokens(redeem, std::move(redeem_callback));
      return;
    }
  }

  // we processed all publishers
  std::move(callback).Run(mojom::Result::OK);
}

void Unblinded::TokenProcessed(const std::string& contribution_id,
                               const std::string& publisher_key,
                               bool final_publisher,
                               ResultCallback callback,
                               mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Tokens were not processed correctly";
    std::move(callback).Run(mojom::Result::RETRY);
    return;
  }

  engine_->database()->UpdateContributionInfoContributedAmount(
      contribution_id, publisher_key,
      base::BindOnce(&Unblinded::ContributionAmountSaved,
                     weak_factory_.GetWeakPtr(), contribution_id,
                     final_publisher, std::move(callback)));
}

void Unblinded::ContributionAmountSaved(const std::string& contribution_id,
                                        bool final_publisher,
                                        ResultCallback callback,
                                        mojom::Result result) {
  if (final_publisher) {
    std::move(callback).Run(result);
    return;
  }

  std::move(callback).Run(mojom::Result::RETRY_LONG);
}

void Unblinded::Retry(const std::vector<mojom::CredsBatchType>& types,
                      mojom::ContributionInfoPtr contribution,
                      ResultCallback callback) {
  if (!contribution) {
    engine_->LogError(FROM_HERE) << "Contribution is null";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  const bool is_not_tokens =
      contribution->processor != mojom::ContributionProcessor::BRAVE_TOKENS;

  const bool is_not_uphold_ac =
      contribution->processor == mojom::ContributionProcessor::UPHOLD &&
      contribution->type != mojom::RewardsType::AUTO_CONTRIBUTE;

  if (is_not_tokens && is_not_uphold_ac) {
    engine_->LogError(FROM_HERE) << "Retry is not for this func";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  switch (contribution->step) {
    case mojom::ContributionStep::STEP_START: {
      Start(types, contribution->contribution_id, std::move(callback));
      return;
    }
    case mojom::ContributionStep::STEP_PREPARE: {
      ProcessTokens(types, contribution->contribution_id, std::move(callback));
      return;
    }
    case mojom::ContributionStep::STEP_RESERVE: {
      std::string contribution_id = contribution->contribution_id;
      engine_->database()->GetReservedUnblindedTokens(
          contribution_id,
          base::BindOnce(&Unblinded::OnReservedUnblindedTokensForRetryAttempt,
                         weak_factory_.GetWeakPtr(), std::move(types),
                         std::move(contribution), std::move(callback)));
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
      NOTREACHED_IN_MIGRATION();
      return;
    }
  }
}

void Unblinded::OnReservedUnblindedTokensForRetryAttempt(
    std::vector<mojom::CredsBatchType> types,
    mojom::ContributionInfoPtr contribution,
    ResultCallback callback,
    std::vector<mojom::UnblindedTokenPtr> unblinded_tokens) {
  if (unblinded_tokens.empty()) {
    engine_->LogError(FROM_HERE) << "Token list is empty";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  if (!contribution) {
    engine_->LogError(FROM_HERE)
        << "Contribution was not converted successfully";
    std::move(callback).Run(mojom::Result::FAILED);
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

  PreparePublishers(converted_list, std::move(contribution), types,
                    std::move(callback));
}

std::string Unblinded::GetStatisticalVotingWinnerForTesting(
    double dart,
    double amount,
    const std::vector<mojom::ContributionPublisherPtr>& publisher_list) {
  return GetStatisticalVotingWinner(dart, amount, publisher_list);
}

}  // namespace brave_rewards::internal::contribution
