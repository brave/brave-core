/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contributions/auto_contribute_processor.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/time/time.h"
#include "base/values.h"
#include "bat/ledger/internal/contributions/auto_contribute_calculator.h"
#include "bat/ledger/internal/contributions/contribution_store.h"
#include "bat/ledger/internal/contributions/contribution_token_manager.h"
#include "bat/ledger/internal/contributions/contribution_token_vendor.h"
#include "bat/ledger/internal/contributions/token_contribution_processor.h"
#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/core/bat_ledger_observer.h"
#include "bat/ledger/internal/core/delay_generator.h"
#include "bat/ledger/internal/core/job_store.h"
#include "bat/ledger/internal/core/value_converters.h"
#include "bat/ledger/internal/external_wallet/external_wallet_manager.h"

namespace ledger {

namespace {

constexpr base::TimeDelta kExternalContributionDelay = base::Seconds(450);

constexpr base::TimeDelta kMinRetryDelay = base::Seconds(15);
constexpr base::TimeDelta kMaxRetryDelay = base::Hours(1);

constexpr int kMaxRetries = 3;

struct PublisherState {
  std::string publisher_id;
  double weight = 0;
  int votes = 0;
  double amount = 0;
  bool completed = false;

  auto ToValue() const {
    ValueWriter w;
    w.Write("publisher_id", publisher_id);
    w.Write("weight", weight);
    w.Write("votes", votes);
    w.Write("amount", amount);
    w.Write("completed", completed);
    return w.Finish();
  }

  static auto FromValue(const base::Value& value) {
    StructValueReader<PublisherState> r(value);
    r.Read("publisher_id", &PublisherState::publisher_id);
    r.Read("weight", &PublisherState::weight);
    r.Read("votes", &PublisherState::votes);
    r.Read("amount", &PublisherState::amount);
    r.Read("completed", &PublisherState::completed);
    return r.Finish();
  }
};

enum class ACStatus { kPending, kPurchasing, kPurchased, kSending, kComplete };

std::string StringifyEnum(ACStatus value) {
  switch (value) {
    case ACStatus::kPending:
      return "pending";
    case ACStatus::kPurchasing:
      return "purchasing";
    case ACStatus::kPurchased:
      return "purchased";
    case ACStatus::kSending:
      return "sending";
    case ACStatus::kComplete:
      return "complete";
  }
}

absl::optional<ACStatus> ParseEnum(const EnumString<ACStatus>& s) {
  return s.Match({ACStatus::kPending, ACStatus::kPurchasing,
                  ACStatus::kPurchased, ACStatus::kSending,
                  ACStatus::kComplete});
}

struct ACState {
  ACStatus status = ACStatus::kPending;
  ContributionSource source;
  std::vector<PublisherState> publishers;
  double amount = 0;
  std::string purchase_job_id;
  std::vector<int64_t> reserved_tokens;

  auto ToValue() const {
    ValueWriter w;
    w.Write("status", status);
    w.Write("source", source);
    w.Write("publishers", publishers);
    w.Write("amount", amount);
    w.Write("purchase_job_id", purchase_job_id);
    w.Write("reserved_tokens", reserved_tokens);
    return w.Finish();
  }

  static auto FromValue(const base::Value& value) {
    StructValueReader<ACState> r(value);
    r.Read("status", &ACState::status);
    r.Read("source", &ACState::source);
    r.Read("publishers", &ACState::publishers);
    r.Read("amount", &ACState::amount);
    r.Read("purchase_job_id", &ACState::purchase_job_id);
    r.Read("reserved_tokens", &ACState::reserved_tokens);
    return r.Finish();
  }
};

class ACJob : public ResumableJob<bool, ACState> {
 public:
  static constexpr char kJobType[] = "auto-contribute";

 protected:
  void Resume() override {
    publisher_iter_ = state().publishers.begin();
    switch (state().status) {
      case ACStatus::kPending:
        return AquireTokens();
      case ACStatus::kPurchasing:
        return CompletePurchase();
      case ACStatus::kPurchased:
        return ReserveTokens();
      case ACStatus::kSending:
        return ReserveAllocatedTokens();
      case ACStatus::kComplete:
        return Complete(true);
    }
  }

  void OnStateInvalid() override { Complete(false); }

 private:
  void AquireTokens() {
    if (state().amount <= 0) {
      context().LogInfo(FROM_HERE) << "Auto contribute amount is zero";
      return Complete(true);
    }

    if (state().publishers.empty()) {
      context().LogInfo(FROM_HERE)
          << "No publisher activity for auto contribute";
      return Complete(true);
    }

    switch (state().source) {
      case ContributionSource::kBraveVG:
        ReserveTokens();
        break;
      case ContributionSource::kBraveSKU:
        context().LogError(FROM_HERE)
            << "Cannot perform auto contribute with SKU tokens";
        return CompleteWithError(false, "invalid-token-type");
      case ContributionSource::kExternal:
        context().Get<ExternalWalletManager>().GetBalance().Then(
            ContinueWith(this, &ACJob::OnExternalBalanceRead));
        break;
    }
  }

  void OnExternalBalanceRead(absl::optional<double> balance) {
    if (!balance || *balance <= 0) {
      context().LogInfo(FROM_HERE) << "Insufficient funds for auto "
                                      "contribution";
      return Complete(true);
    }

    state().status = ACStatus::kPurchasing;
    state().purchase_job_id =
        context().Get<ContributionTokenVendor>().StartPurchase(
            std::min(state().amount, *balance));

    SaveState();
    CompletePurchase();
  }

  void CompletePurchase() {
    DCHECK(!state().purchase_job_id.empty());
    context()
        .Get<ContributionTokenVendor>()
        .ResumePurchase(state().purchase_job_id)
        .Then(ContinueWith(this, &ACJob::OnTokensPurchased));
  }

  void OnTokensPurchased(bool success) {
    if (!success) {
      context().LogError(FROM_HERE) << "Error purchasing contribution tokens";
      return CompleteWithError(false, "purchase-error");
    }
    state().status = ACStatus::kPurchased;
    SaveState();
    ReserveTokens();
  }

  void ReserveTokens() {
    context()
        .Get<ContributionTokenManager>()
        .ReserveTokens(GetTokenType(), state().amount)
        .Then(ContinueWith(this, &ACJob::OnTokensReserved));
  }

  void OnTokensReserved(ContributionTokenHold hold) {
    hold_ = std::move(hold);

    if (hold_.tokens().empty()) {
      if (IsExternallyFunded()) {
        context().LogError(FROM_HERE)
            << "Expected SKU auto contribute tokens were not found";
        return CompleteWithError(false, "tokens-not-found");
      }

      context().LogInfo(FROM_HERE) << "No tokens available for auto "
                                      "contribution";
      return Complete(true);
    }

    for (auto& token : hold_.tokens()) {
      state().reserved_tokens.push_back(token.id);
    }

    AutoContributeCalculator::WeightMap weights;
    for (auto& publisher_state : state().publishers) {
      weights[publisher_state.publisher_id] = publisher_state.weight;
    }

    auto votes = context().Get<AutoContributeCalculator>().AllocateVotes(
        weights, hold_.tokens().size());

    for (auto& publisher_state : state().publishers) {
      publisher_state.votes = votes[publisher_state.publisher_id];
    }

    state().status = ACStatus::kSending;
    SaveState();

    SendNext();
  }

  void ReserveAllocatedTokens() {
    context()
        .Get<ContributionTokenManager>()
        .ReserveTokens(state().reserved_tokens)
        .Then(ContinueWith(this, &ACJob::OnAllocatedTokensReserved));
  }

  void OnAllocatedTokensReserved(ContributionTokenHold hold) {
    hold_ = std::move(hold);
    SendNext();
  }

  void SendNext() {
    publisher_iter_ = std::find_if(
        publisher_iter_, state().publishers.end(),
        [](const PublisherState& publisher) { return !publisher.completed; });

    if (publisher_iter_ == state().publishers.end()) {
      return OnAutoContributeCompleted();
    }

    if (publisher_iter_->votes == 0) {
      return OnContributionProcessed(TokenContributionResult::kSuccess);
    }

    context()
        .Get<DelayGenerator>()
        .RandomDelay(FROM_HERE, GetContributionDelay())
        .Then(ContinueWith(this, &ACJob::OnSendNextDelayElapsed));
  }

  void OnSendNextDelayElapsed(base::TimeDelta) {
    DCHECK(publisher_iter_ != state().publishers.end());

    auto publisher_hold = hold_.Split(publisher_iter_->votes);
    double amount = publisher_hold.GetTotalValue();

    publisher_iter_->amount = amount;
    SaveState();

    Contribution contribution{.type = ContributionType::kAutoContribute,
                              .publisher_id = publisher_iter_->publisher_id,
                              .amount = amount,
                              .source = GetContributionSource()};

    context()
        .Get<TokenContributionProcessor>()
        .ProcessContribution(contribution, std::move(publisher_hold))
        .Then(ContinueWith(this, &ACJob::OnContributionProcessed));
  }

  void OnContributionProcessed(TokenContributionResult result) {
    DCHECK(publisher_iter_ != state().publishers.end());

    if (result != TokenContributionResult::kSuccess) {
      context().LogError(FROM_HERE) << "Unable to send contribution";

      if (ShouldRetryContribution()) {
        context()
            .Get<DelayGenerator>()
            .Delay(FROM_HERE, backoff_.GetNextDelay())
            .Then(ContinueWith(this, &ACJob::OnRetryDelayElapsed));
        return;
      }

      context().LogError(FROM_HERE)
          << "Contribution failed after " << backoff_.count() << " retries";
    }

    backoff_.Reset();

    publisher_iter_->completed = true;
    SaveState();

    SendNext();
  }

  void OnRetryDelayElapsed(base::TimeDelta) { SendNext(); }

  void OnAutoContributeCompleted() {
    state().status = ACStatus::kComplete;
    SaveState();

    base::flat_map<std::string, double> publisher_amounts;
    for (auto& entry : state().publishers) {
      publisher_amounts[entry.publisher_id] = entry.amount;
    }

    context().Get<ContributionStore>().SaveCompletedAutoContribute(
        publisher_amounts, state().source);

    context().Get<BATLedgerObserver>().OnAutoContributeCompleted(
        state().amount);

    Complete(true);
  }

  bool ShouldRetryContribution() {
    // For externally-funded AC, BAT has already been transferred from the
    // user's external wallet in order to purchase contribution tokens. We must
    // continue trying to send the contribution until it completes successfully.
    if (IsExternallyFunded()) {
      return true;
    }

    return backoff_.count() > kMaxRetries;
  }

  bool IsExternallyFunded() {
    return state().source == ContributionSource::kExternal;
  }

  ContributionTokenType GetTokenType() {
    switch (state().source) {
      case ContributionSource::kBraveVG:
        return ContributionTokenType::kVG;
      case ContributionSource::kBraveSKU:
      case ContributionSource::kExternal:
        return ContributionTokenType::kSKU;
    }
  }

  ContributionSource GetContributionSource() {
    switch (state().source) {
      case ContributionSource::kExternal:
        return ContributionSource::kBraveSKU;
      default:
        return state().source;
    }
  }

  base::TimeDelta GetContributionDelay() {
    return IsExternallyFunded() ? kExternalContributionDelay
                                : kBackgroundContributionDelay;
  }

  ContributionTokenHold hold_;
  std::vector<PublisherState>::iterator publisher_iter_;
  BackoffDelay backoff_{kMinRetryDelay, kMaxRetryDelay};
};

}  // namespace

std::string AutoContributeProcessor::StartContributions(
    ContributionSource source,
    const std::vector<PublisherActivity>& activity,
    int min_visits,
    base::TimeDelta min_duration,
    double amount) {
  auto weights = context().Get<AutoContributeCalculator>().CalculateWeights(
      activity, min_visits, min_duration);

  ACState state{.source = source, .amount = amount};
  for (auto& pair : weights) {
    state.publishers.push_back(
        {.publisher_id = pair.first, .weight = pair.second});
  }

  return context().Get<JobStore>().InitializeJobState<ACJob>(state);
}

Future<bool> AutoContributeProcessor::ResumeContributions(
    const std::string& job_id) {
  return context().StartJob<ACJob>(job_id);
}

}  // namespace ledger
