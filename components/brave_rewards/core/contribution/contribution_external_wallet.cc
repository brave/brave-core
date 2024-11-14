/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "brave/components/brave_rewards/core/bitflyer/bitflyer.h"
#include "brave/components/brave_rewards/core/contribution/contribution.h"
#include "brave/components/brave_rewards/core/contribution/contribution_external_wallet.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/gemini/gemini.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/uphold/uphold.h"

namespace brave_rewards::internal::contribution {

ContributionExternalWallet::ContributionExternalWallet(RewardsEngine& engine)
    : engine_(engine) {}

ContributionExternalWallet::~ContributionExternalWallet() = default;

void ContributionExternalWallet::Process(const std::string& contribution_id,
                                         ResultCallback callback) {
  if (contribution_id.empty()) {
    engine_->LogError(FROM_HERE) << "Contribution id is empty";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  engine_->database()->GetContributionInfo(
      contribution_id,
      base::BindOnce(&ContributionExternalWallet::ContributionInfo,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void ContributionExternalWallet::ContributionInfo(
    ResultCallback callback,
    mojom::ContributionInfoPtr contribution) {
  if (!contribution) {
    engine_->LogError(FROM_HERE) << "Contribution is null";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  mojom::ExternalWalletPtr wallet;
  switch (contribution->processor) {
    case mojom::ContributionProcessor::BITFLYER:
      wallet =
          engine_->bitflyer()->GetWalletIf({mojom::WalletStatus::kConnected});
      break;
    case mojom::ContributionProcessor::GEMINI:
      wallet =
          engine_->gemini()->GetWalletIf({mojom::WalletStatus::kConnected});
      break;
    case mojom::ContributionProcessor::UPHOLD:
      wallet =
          engine_->uphold()->GetWalletIf({mojom::WalletStatus::kConnected});
      break;
    default:
      break;
  }

  if (!wallet) {
    engine_->LogError(FROM_HERE) << "Unexpected wallet status";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  if (contribution->type == mojom::RewardsType::AUTO_CONTRIBUTE) {
    engine_->LogError(FROM_HERE) << "AC is disabled";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  bool single_publisher = contribution->publishers.size() == 1;

  for (const auto& publisher : contribution->publishers) {
    if (publisher->total_amount == publisher->contributed_amount) {
      continue;
    }

    engine_->publisher()->GetServerPublisherInfo(
        publisher->publisher_key,
        base::BindOnce(&ContributionExternalWallet::OnServerPublisherInfo,
                       weak_factory_.GetWeakPtr(),
                       contribution->contribution_id, publisher->total_amount,
                       contribution->type, contribution->processor,
                       single_publisher, std::move(callback)));
    return;
  }

  // we processed all publishers
  std::move(callback).Run(mojom::Result::OK);
}

void ContributionExternalWallet::OnServerPublisherInfo(
    const std::string& contribution_id,
    double amount,
    mojom::RewardsType type,
    mojom::ContributionProcessor processor,
    bool single_publisher,
    ResultCallback callback,
    mojom::ServerPublisherInfoPtr info) {
  if (!info) {
    engine_->LogError(FROM_HERE) << "Publisher not found";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  bool publisher_verified = false;
  switch (info->status) {
    case mojom::PublisherStatus::UPHOLD_VERIFIED:
      publisher_verified = processor == mojom::ContributionProcessor::UPHOLD;
      break;
    case mojom::PublisherStatus::BITFLYER_VERIFIED:
      publisher_verified = processor == mojom::ContributionProcessor::BITFLYER;
      break;
    case mojom::PublisherStatus::GEMINI_VERIFIED:
      publisher_verified = processor == mojom::ContributionProcessor::GEMINI;
      break;
    default:
      break;
  }

  if (!publisher_verified) {
    // NOTE: At this point we assume that the user has a connected wallet for
    // the specified |provider| and that the wallet balance is non-zero. We also
    // assume that the user cannot have two connected wallets at the same time.
    // We can then infer that no other external wallet will be able to service
    // this contribution item, and we can safely error out.
    engine_->Log(FROM_HERE) << "Publisher not verified";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto start_callback = base::BindOnce(&ContributionExternalWallet::Completed,
                                       weak_factory_.GetWeakPtr(),
                                       single_publisher, std::move(callback));

  switch (processor) {
    case mojom::ContributionProcessor::UPHOLD:
      engine_->uphold()->StartContribution(contribution_id, std::move(info),
                                           amount, std::move(start_callback));
      break;
    case mojom::ContributionProcessor::BITFLYER:
      engine_->bitflyer()->StartContribution(contribution_id, std::move(info),
                                             amount, std::move(start_callback));
      break;
    case mojom::ContributionProcessor::GEMINI:
      engine_->gemini()->StartContribution(contribution_id, std::move(info),
                                           amount, std::move(start_callback));
      break;
    default:
      engine_->LogError(FROM_HERE) << "Contribution processor not supported";
      break;
  }
}

void ContributionExternalWallet::Completed(bool single_publisher,
                                           ResultCallback callback,
                                           mojom::Result result) {
  if (single_publisher) {
    std::move(callback).Run(result);
    return;
  }

  std::move(callback).Run(mojom::Result::RETRY);
}

void ContributionExternalWallet::Retry(mojom::ContributionInfoPtr contribution,
                                       ResultCallback callback) {
  Process(contribution->contribution_id, std::move(callback));
}

}  // namespace brave_rewards::internal::contribution
