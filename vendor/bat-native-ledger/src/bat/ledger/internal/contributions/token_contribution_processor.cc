/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contributions/token_contribution_processor.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/core/job_store.h"
#include "bat/ledger/internal/credentials/credentials_redeem.h"
#include "bat/ledger/internal/endpoint/promotion/promotion_server.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/payments/payment_service.h"
#include "bat/ledger/internal/publisher/publisher_service.h"

namespace ledger {

namespace {

mojom::RewardsType ContributionTypeToRewardsType(ContributionType type) {
  switch (type) {
    case ContributionType::kOneTime:
      return mojom::RewardsType::ONE_TIME_TIP;
    case ContributionType::kRecurring:
      return mojom::RewardsType::RECURRING_TIP;
    case ContributionType::kAutoContribute:
      return mojom::RewardsType::AUTO_CONTRIBUTE;
  }
}

class ProcessJob : public BATLedgerJob<TokenContributionResult> {
 public:
  void Start(const Contribution& contribution) {
    DCHECK_GT(contribution.amount, 0.0);

    contribution_ = contribution;

    context()
        .Get<ContributionTokenManager>()
        .ReserveTokens(GetContributionTokenType(), contribution_.amount)
        .Then(ContinueWith(this, &ProcessJob::OnTokensReserved));
  }

  void Start(const Contribution& contribution,
             ContributionTokenManager::Hold hold) {
    contribution_ = contribution;
    OnTokensReserved(std::move(hold));
  }

 private:
  void OnTokensReserved(ContributionTokenHold hold) {
    hold_ = std::move(hold);

    double total_value = hold_.GetTotalValue();
    if (total_value < contribution_.amount) {
      context().LogError(FROM_HERE)
          << "Insufficient tokens reserved for contribution";
      return Complete(TokenContributionResult::kInsufficientTokens);
    }

    // The contribution amount could differ slightly from the requested amount
    // based on the per-token value. Update the contribution amount to reflect
    // the value of the tokens being sent.
    contribution_.amount = total_value;

    context()
        .Get<PublisherService>()
        .GetPublisher(contribution_.publisher_id)
        .Then(ContinueWith(this, &ProcessJob::OnPublisherLoaded));
  }

  void OnPublisherLoaded(absl::optional<Publisher> publisher) {
    if (!publisher || !publisher->registered) {
      context().LogError(FROM_HERE) << "Publisher is not registered";
      return Complete(TokenContributionResult::kPublisherNotRegistered);
    }

    if (GetContributionTokenType() == ContributionTokenType::kSKU) {
      RedeemVotes();
    } else {
      RedeemGrantTokens();
    }
  }

  void RedeemVotes() {
    std::vector<PaymentVote> votes;
    for (auto& token : hold_.tokens()) {
      votes.push_back({.unblinded_token = token.unblinded_token,
                       .public_key = token.public_key});
    }

    context()
        .Get<PaymentService>()
        .PostPublisherVotes(contribution_.publisher_id, GetVoteType(),
                            std::move(votes))
        .Then(ContinueWith(this, &ProcessJob::OnContributionProcessed));
  }

  void RedeemGrantTokens() {
    std::vector<mojom::UnblindedToken> ut_list;
    for (auto& token : hold_.tokens()) {
      mojom::UnblindedToken ut;
      ut.id = token.id;
      ut.token_value = token.unblinded_token;
      ut.public_key = token.public_key;
      ut_list.push_back(std::move(ut));
    }

    credential::CredentialsRedeem redeem;
    redeem.publisher_key = contribution_.publisher_id;
    redeem.type = ContributionTypeToRewardsType(contribution_.type);
    redeem.processor = mojom::ContributionProcessor::NONE;
    redeem.token_list = std::move(ut_list);

    promotion_server_.reset(
        new endpoint::PromotionServer(context().GetLedgerImpl()));

    promotion_server_->post_suggestions()->Request(
        redeem, ContinueWithLambda(this, &ProcessJob::OnGrantTokensRedeemed));
  }

  void OnGrantTokensRedeemed(mojom::Result result) {
    OnContributionProcessed(result == mojom::Result::LEDGER_OK);
  }

  void OnContributionProcessed(bool success) {
    if (!success) {
      context().LogError(FROM_HERE) << "Unable to redeem contribution tokens";
      return Complete(TokenContributionResult::kRedeemError);
    }

    const std::string id = context().Get<JobStore>().AddCompletedState(
        "token-contribution", contribution_);

    hold_.OnTokensRedeemed(id);

    Complete(TokenContributionResult::kSuccess);
  }

  ContributionTokenType GetContributionTokenType() {
    switch (contribution_.source) {
      case ContributionSource::kBraveVG:
        return ContributionTokenType::kVG;
      case ContributionSource::kBraveSKU:
        return ContributionTokenType::kSKU;
      default:
        NOTREACHED();
        return ContributionTokenType::kVG;
    }
  }

  PaymentVoteType GetVoteType() {
    switch (contribution_.type) {
      case ContributionType::kOneTime:
        return PaymentVoteType::kOneOffTip;
      case ContributionType::kRecurring:
        return PaymentVoteType::kRecurringTip;
      case ContributionType::kAutoContribute:
        return PaymentVoteType::kAutoContribute;
    }
  }

  Contribution contribution_;
  ContributionTokenHold hold_;
  std::unique_ptr<endpoint::PromotionServer> promotion_server_;
};

}  // namespace

Future<TokenContributionResult> TokenContributionProcessor::ProcessContribution(
    const Contribution& contribution) {
  return context().StartJob<ProcessJob>(contribution);
}

Future<TokenContributionResult> TokenContributionProcessor::ProcessContribution(
    const Contribution& contribution,
    ContributionTokenManager::Hold hold) {
  return context().StartJob<ProcessJob>(contribution, std::move(hold));
}

}  // namespace ledger
