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
    braveledger_uphold::Uphold* uphold) :
    ledger_(ledger),
    uphold_(uphold) {
  DCHECK(ledger_ && uphold_);
}

ContributionExternalWallet::~ContributionExternalWallet() = default;

void ContributionExternalWallet::Process(
    const std::string& contribution_id,
    ledger::ResultCallback callback) {
  if (contribution_id.empty()) {
    BLOG(0, "Contribution id is empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto wallets = ledger_->ledger_client()->GetExternalWallets();

  if (wallets.empty()) {
    BLOG(0, "No external wallets");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto wallet = braveledger_uphold::GetWallet(std::move(wallets));

  if (!wallet) {
    BLOG(0, "External wallet null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback = std::bind(&ContributionExternalWallet::ContributionInfo,
      this,
      _1,
      *wallet,
      callback);
  ledger_->database()->GetContributionInfo(contribution_id, get_callback);
}

void ContributionExternalWallet::ContributionInfo(
    ledger::ContributionInfoPtr contribution,
    const ledger::ExternalWallet& wallet,
    ledger::ResultCallback callback) {
  if (!contribution) {
    BLOG(0, "Contribution is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  // In this phase we only support one wallet
  // so we will just always pick uphold.
  // In the future we will allow user to pick which wallet to use via UI
  // and then we will extend this function
  if (wallet.token.empty() ||
      wallet.status != ledger::WalletStatus::VERIFIED) {
    BLOG(0, "Wallet token is empty/wallet is not verified. Wallet status: "
        << wallet.status);
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (contribution->type == ledger::RewardsType::AUTO_CONTRIBUTE) {
    ledger_->contribution()->SKUAutoContribution(
        contribution->contribution_id,
        ledger::ExternalWallet::New(wallet),
        callback);
    return;
  }

  bool single_publisher = contribution->publishers.size() == 1;

  for (const auto& publisher : contribution->publishers) {
    if (publisher->total_amount == publisher->contributed_amount) {
      continue;
    }

    auto get_callback =
        std::bind(&ContributionExternalWallet::OnServerPublisherInfo,
          this,
          _1,
          contribution->contribution_id,
          publisher->total_amount,
          contribution->type,
          single_publisher,
          callback);

    ledger_->publisher()->GetServerPublisherInfo(
        publisher->publisher_key,
        get_callback);
    return;
  }

  // we processed all publishers
  callback(ledger::Result::LEDGER_OK);
}

void ContributionExternalWallet::OnSavePendingContribution(
    const ledger::Result result) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Problem saving pending");
  }
  ledger_->ledger_client()->PendingContributionSaved(result);
}

void ContributionExternalWallet::OnServerPublisherInfo(
    ledger::ServerPublisherInfoPtr info,
    const std::string& contribution_id,
    const double amount,
    const ledger::RewardsType type,
    const bool single_publisher,
    ledger::ResultCallback callback) {
  if (!info) {
    BLOG(0, "Publisher not found");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (info->status != ledger::PublisherStatus::VERIFIED) {
    BLOG(1, "Publisher not verified");

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

    ledger_->database()->SavePendingContribution(
        std::move(list),
        save_callback);
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto uphold_callback = std::bind(&ContributionExternalWallet::Completed,
      this,
      _1,
      single_publisher,
      callback);

  uphold_->StartContribution(
      contribution_id,
      std::move(info),
      amount,
      uphold_callback);
}

void ContributionExternalWallet::Completed(
    const ledger::Result result,
    const bool single_publisher,
    ledger::ResultCallback callback) {
  if (single_publisher) {
    callback(result);
    return;
  }

  callback(ledger::Result::RETRY);
}

void ContributionExternalWallet::Retry(
    ledger::ContributionInfoPtr contribution,
    ledger::ResultCallback callback) {
  Process(contribution->contribution_id, callback);
}

}  // namespace braveledger_contribution
