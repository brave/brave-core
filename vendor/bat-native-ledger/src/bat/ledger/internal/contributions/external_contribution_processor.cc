/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contributions/external_contribution_processor.h"

#include <string>
#include <utility>

#include "bat/ledger/internal/contributions/contribution_fee_processor.h"
#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/core/job_store.h"
#include "bat/ledger/internal/core/value_converters.h"
#include "bat/ledger/internal/external_wallet/external_wallet_manager.h"
#include "bat/ledger/internal/publisher/publisher_service.h"

namespace ledger {

namespace {

constexpr double kTransferFee = 0.05;

struct JobState {
  Contribution contribution;
  ExternalWalletProvider provider;
  std::string transaction_id;

  auto ToValue() const {
    ValueWriter w;
    w.Write("contribution", contribution);
    w.Write("provider", provider);
    w.Write("transaction_id", transaction_id);
    return w.Finish();
  }
};

class ProcessJob : public BATLedgerJob<ExternalContributionResult> {
 public:
  void Start(const Contribution& contribution) {
    DCHECK_GT(contribution.amount, 0.0);
    DCHECK(contribution.type != ContributionType::kAutoContribute);
    DCHECK(contribution.source == ContributionSource::kExternal);

    contribution_ = contribution;
    fee_ = contribution_.amount * kTransferFee;
    amount_ = contribution_.amount - fee_;

    context().Get<ExternalWalletManager>().GetBalance().Then(
        ContinueWith(this, &ProcessJob::OnBalanceFetched));
  }

 private:
  void OnBalanceFetched(absl::optional<double> balance) {
    if (!balance) {
      context().LogError(FROM_HERE) << "Unable to read external wallet balance";
      return Complete(ExternalContributionResult::kBalanceUnavailable);
    }

    if (*balance < contribution_.amount) {
      context().LogError(FROM_HERE) << "Insufficient funds for contribution";
      return Complete(ExternalContributionResult::kInsufficientFunds);
    }

    context()
        .Get<PublisherService>()
        .GetPublisher(contribution_.publisher_id)
        .Then(ContinueWith(this, &ProcessJob::OnPublisherLoaded));
  }

  void OnPublisherLoaded(absl::optional<Publisher> publisher) {
    if (!publisher) {
      context().LogError(FROM_HERE) << "Unable to load publisher info";
      return Complete(ExternalContributionResult::kNoPublisherAddress);
    }

    std::string publisher_address = GetPublisherAddress(*publisher);
    if (publisher_address.empty()) {
      context().LogError(FROM_HERE) << "Publisher does not have a matching "
                                    << "wallet provider address";
      return Complete(ExternalContributionResult::kNoPublisherAddress);
    }

    context()
        .Get<ExternalWalletManager>()
        .TransferBAT(publisher_address, amount_)
        .Then(ContinueWith(this, &ProcessJob::OnTransferCompleted));
  }

  void OnTransferCompleted(
      absl::optional<ExternalWalletTransferResult> result) {
    if (!result) {
      context().LogError(FROM_HERE) << "Unable to send contribution to "
                                       "publisher";
      return Complete(ExternalContributionResult::kTransferError);
    }

    const std::string id = context().Get<JobStore>().AddCompletedState(
        "external-contribution",
        JobState{.contribution = contribution_,
                 .provider = result->provider,
                 .transaction_id = result->transaction_id});

    context().Get<ContributionFeeProcessor>().SendContributionFee(id, fee_);

    Complete(ExternalContributionResult::kSuccess);
  }

  std::string GetPublisherAddress(const Publisher& publisher) {
    auto wallet = context().Get<ExternalWalletManager>().GetExternalWallet();
    if (!wallet) {
      return "";
    }

    for (auto& publisher_wallet : publisher.wallets) {
      if (publisher_wallet.provider == wallet->provider) {
        return publisher_wallet.address;
      }
    }

    return "";
  }

  Contribution contribution_;
  double amount_;
  double fee_;
};

}  // namespace

Future<ExternalContributionResult>
ExternalContributionProcessor::ProcessContribution(
    const Contribution& contribution) {
  return context().StartJob<ProcessJob>(contribution);
}

}  // namespace ledger
