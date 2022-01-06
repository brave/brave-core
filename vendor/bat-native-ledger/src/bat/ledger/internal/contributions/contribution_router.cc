/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contributions/contribution_router.h"

#include <string>
#include <utility>

#include "bat/ledger/internal/contributions/contribution_data.h"
#include "bat/ledger/internal/contributions/contribution_store.h"
#include "bat/ledger/internal/contributions/contribution_token_manager.h"
#include "bat/ledger/internal/contributions/external_contribution_processor.h"
#include "bat/ledger/internal/contributions/token_contribution_processor.h"
#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/core/bat_ledger_observer.h"
#include "bat/ledger/internal/external_wallet/external_wallet_manager.h"

namespace ledger {

namespace {

class SendJob : public BATLedgerJob<bool> {
 public:
  void Start(Contribution contribution, bool maybe_save_pending) {
    contribution_ = std::move(contribution);
    maybe_save_pending_ = maybe_save_pending;

    DCHECK(!contribution_.publisher_id.empty());

    if (contribution_.amount <= 0) {
      context().LogInfo(FROM_HERE) << "Attempting to send a contribution with "
                                      "zero amount";
      return Complete(true);
    }

    switch (contribution_.source) {
      case ContributionSource::kBraveVG:
      case ContributionSource::kBraveSKU:
        context()
            .Get<TokenContributionProcessor>()
            .ProcessContribution(contribution_)
            .Then(ContinueWith(this, &SendJob::OnTokenContributionSent));
        break;
      case ContributionSource::kExternal:
        context()
            .Get<ExternalContributionProcessor>()
            .ProcessContribution(contribution_)
            .Then(ContinueWith(this, &SendJob::OnExternalContributionSent));
        break;
    }
  }

 private:
  void OnTokenContributionSent(TokenContributionResult result) {
    switch (result) {
      case TokenContributionResult::kSuccess:
        return SaveCompletedContribution();
      case TokenContributionResult::kPublisherNotRegistered:
        return MaybeSavePending();
      default:
        return Complete(false);
    }
  }

  void OnExternalContributionSent(ExternalContributionResult result) {
    switch (result) {
      case ExternalContributionResult::kSuccess:
        return SaveCompletedContribution();
      case ExternalContributionResult::kNoPublisherAddress:
        return MaybeSavePending();
      default:
        return Complete(false);
    }
  }

  void SaveCompletedContribution() {
    context().Get<ContributionStore>().SaveCompletedContribution(contribution_);
    context().Get<BATLedgerObserver>().OnContributionCompleted(
        contribution_.amount);
    Complete(true);
  }

  void MaybeSavePending() {
    if (!maybe_save_pending_) {
      return Complete(false);
    }

    if (contribution_.type != ContributionType::kOneTime) {
      NOTREACHED();
      return Complete(false);
    }

    context()
        .Get<ContributionStore>()
        .SavePendingContribution(contribution_.publisher_id,
                                 contribution_.amount)
        .Then(ContinueWith(this, &SendJob::OnPendingTipSaved));
  }

  void OnPendingTipSaved(bool success) { Complete(success); }

  Contribution contribution_;
  bool maybe_save_pending_ = false;
};

}  // namespace

Future<bool> ContributionRouter::SendContribution(
    ContributionType contribution_type,
    const std::string& publisher_id,
    double amount) {
  Contribution contribution{.type = contribution_type,
                            .publisher_id = publisher_id,
                            .amount = amount,
                            .source = GetCurrentSource()};

  return context().StartJob<SendJob>(std::move(contribution), false);
}

Future<bool> ContributionRouter::SendOrSavePendingContribution(
    const std::string& publisher_id,
    double amount) {
  Contribution contribution{.type = ContributionType::kOneTime,
                            .publisher_id = publisher_id,
                            .amount = amount,
                            .source = GetCurrentSource()};

  return context().StartJob<SendJob>(std::move(contribution), true);
}

Future<absl::optional<double>> ContributionRouter::GetAvailableBalance() {
  if (context().Get<ExternalWalletManager>().HasExternalWallet()) {
    return context().Get<ExternalWalletManager>().GetBalance();
  }

  return context()
      .Get<ContributionTokenManager>()
      .GetAvailableTokenBalance(ContributionTokenType::kVG)
      .Then(base::BindOnce([](double d) { return absl::optional<double>(d); }));
}

ContributionSource ContributionRouter::GetCurrentSource() {
  return context().Get<ExternalWalletManager>().HasExternalWallet()
             ? ContributionSource::kExternal
             : ContributionSource::kBraveVG;
}

}  // namespace ledger
