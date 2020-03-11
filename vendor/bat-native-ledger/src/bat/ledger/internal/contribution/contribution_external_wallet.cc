/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/contribution/contribution_external_wallet.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/uphold/uphold_util.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace braveledger_contribution {

ContributionExternalWallet::ContributionExternalWallet(
    bat_ledger::LedgerImpl* ledger,
    Contribution* contribution,
    braveledger_uphold::Uphold* uphold) :
    ledger_(ledger),
    contribution_(contribution),
    uphold_(uphold) {
  DCHECK(ledger_ && contribution_ && uphold_);
}

ContributionExternalWallet::~ContributionExternalWallet() = default;

void ContributionExternalWallet::Process(const std::string& contribution_id) {
  auto wallets_callback = std::bind(
      &ContributionExternalWallet::OnExternalWallets,
      this,
      contribution_id,
      _1);

  // Check if we have token
  ledger_->GetExternalWallets(wallets_callback);
}

void ContributionExternalWallet::OnExternalWallets(
    const std::string& contribution_id,
    std::map<std::string, ledger::ExternalWalletPtr> wallets) {
  if (wallets.size() == 0) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "No external wallets";
    ledger_->UpdateContributionInfoStepAndCount(
        contribution_id,
        ledger::ContributionStep::STEP_FAILED,
        -1,
        [](const ledger::Result _){});
    return;
  }

  ledger::ExternalWalletPtr wallet =
      braveledger_uphold::GetWallet(std::move(wallets));

  ledger_->GetContributionInfo(
      contribution_id,
      std::bind(&ContributionExternalWallet::ContributionInfo,
                this,
                _1,
                *wallet));
}

void ContributionExternalWallet::ContributionInfo(
    ledger::ContributionInfoPtr contribution,
    const ledger::ExternalWallet& wallet) {
  if (!contribution) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Contribution is null";
    return;
  }

  // In this phase we only support one wallet
  // so we will just always pick uphold.
  // In the future we will allow user to pick which wallet to use via UI
  // and then we will extend this function
  if (wallet.token.empty() ||
      wallet.status != ledger::WalletStatus::VERIFIED) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
        << "Wallet token is empty/wallet is not verified " << wallet.status;
    ledger_->ContributionCompleted(
        ledger::Result::LEDGER_ERROR,
        contribution->amount,
        contribution->contribution_id,
        contribution->type);
  }

  if (contribution->type == ledger::RewardsType::AUTO_CONTRIBUTE) {
    contribution_->SKUAutoContribution(
        contribution->contribution_id,
        ledger::ExternalWallet::New(wallet));
    return;
  }

  for (const auto& publisher : contribution->publishers) {
    auto callback =
        std::bind(&ContributionExternalWallet::OnServerPublisherInfo,
          this,
          _1,
          contribution->contribution_id,
          publisher->total_amount,
          wallet,
          contribution->type);

    ledger_->GetServerPublisherInfo(publisher->publisher_key, callback);
  }
}

void ContributionExternalWallet::OnSavePendingContribution(
    const ledger::Result result) {
  ledger_->PendingContributionSaved(result);
}

void ContributionExternalWallet::OnServerPublisherInfo(
    ledger::ServerPublisherInfoPtr info,
    const std::string& contribution_id,
    const double amount,
    const ledger::ExternalWallet& wallet,
    const ledger::RewardsType type) {
  if (!info) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Publisher not found";
    ledger_->ContributionCompleted(
        ledger::Result::LEDGER_ERROR,
        amount,
        contribution_id,
        type);
    return;
  }

  if (info->status != ledger::PublisherStatus::VERIFIED) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Publisher not verified";

    auto save_callback =
        std::bind(&ContributionExternalWallet::OnSavePendingContribution,
            this,
            _1);

    auto contribution = ledger::PendingContribution::New();
    contribution->publisher_key = info->publisher_key;
    contribution->amount = amount;
    contribution->type = type;

    ledger::PendingContributionList list;
    list.push_back(std::move(contribution));

    ledger_->SavePendingContribution(std::move(list), save_callback);

    ledger_->ContributionCompleted(
        ledger::Result::LEDGER_ERROR,
        amount,
        contribution_id,
        type);
    return;
  }

  auto completed_callback =
      std::bind(&ContributionExternalWallet::Completed,
          this,
          _1,
          amount,
          contribution_id,
          type);

  uphold_->StartContribution(
      contribution_id,
      std::move(info),
      amount,
      ledger::ExternalWallet::New(wallet),
      completed_callback);
}

void ContributionExternalWallet::Completed(
    const ledger::Result result,
    const double amount,
    const std::string& contribution_id,
    const ledger::RewardsType type) {
  ledger_->ContributionCompleted(result, amount, contribution_id, type);
}

}  // namespace braveledger_contribution
