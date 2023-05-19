/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "brave/components/brave_rewards/core/bitflyer/bitflyer.h"
#include "brave/components/brave_rewards/core/bitflyer/bitflyer_util.h"
#include "brave/components/brave_rewards/core/contribution/contribution.h"
#include "brave/components/brave_rewards/core/contribution/contribution_external_wallet.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/gemini/gemini.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/logging/logging.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"
#include "brave/components/brave_rewards/core/uphold/uphold.h"
#include "brave/components/brave_rewards/core/uphold/uphold_util.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace brave_rewards::internal::contribution {

void ContributionExternalWallet::Process(const std::string& contribution_id,
                                         LegacyResultCallback callback) {
  if (contribution_id.empty()) {
    BLOG(0, "Contribution id is empty");
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback = std::bind(&ContributionExternalWallet::ContributionInfo,
                                this, _1, callback);
  ledger().database()->GetContributionInfo(contribution_id, get_callback);
}

void ContributionExternalWallet::ContributionInfo(
    mojom::ContributionInfoPtr contribution,
    LegacyResultCallback callback) {
  if (!contribution) {
    BLOG(0, "Contribution is null");
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  mojom::ExternalWalletPtr wallet;
  switch (contribution->processor) {
    case mojom::ContributionProcessor::BITFLYER:
      wallet =
          ledger().bitflyer()->GetWalletIf({mojom::WalletStatus::kConnected});
      break;
    case mojom::ContributionProcessor::GEMINI:
      wallet =
          ledger().gemini()->GetWalletIf({mojom::WalletStatus::kConnected});
      break;
    case mojom::ContributionProcessor::UPHOLD:
      wallet =
          ledger().uphold()->GetWalletIf({mojom::WalletStatus::kConnected});
      break;
    default:
      break;
  }

  if (!wallet) {
    BLOG(0, "Unexpected wallet status!");
    return callback(mojom::Result::LEDGER_ERROR);
  }

  if (contribution->type == mojom::RewardsType::AUTO_CONTRIBUTE) {
    ledger().contribution()->SKUAutoContribution(contribution->contribution_id,
                                                 wallet->type, callback);
    return;
  }

  bool single_publisher = contribution->publishers.size() == 1;

  for (const auto& publisher : contribution->publishers) {
    if (publisher->total_amount == publisher->contributed_amount) {
      continue;
    }

    auto get_callback =
        std::bind(&ContributionExternalWallet::OnServerPublisherInfo, this, _1,
                  contribution->contribution_id, publisher->total_amount,
                  contribution->type, contribution->processor, single_publisher,
                  callback);

    ledger().publisher()->GetServerPublisherInfo(publisher->publisher_key,
                                                 get_callback);
    return;
  }

  // we processed all publishers
  callback(mojom::Result::LEDGER_OK);
}

void ContributionExternalWallet::OnServerPublisherInfo(
    mojom::ServerPublisherInfoPtr info,
    const std::string& contribution_id,
    double amount,
    mojom::RewardsType type,
    mojom::ContributionProcessor processor,
    bool single_publisher,
    LegacyResultCallback callback) {
  if (!info) {
    BLOG(0, "Publisher not found");
    callback(mojom::Result::LEDGER_ERROR);
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
    BLOG(1, "Publisher not verified");
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  auto start_callback = std::bind(&ContributionExternalWallet::Completed, this,
                                  _1, single_publisher, callback);

  switch (processor) {
    case mojom::ContributionProcessor::UPHOLD:
      ledger().uphold()->StartContribution(contribution_id, std::move(info),
                                           amount, start_callback);
      break;
    case mojom::ContributionProcessor::BITFLYER:
      ledger().bitflyer()->StartContribution(contribution_id, std::move(info),
                                             amount, start_callback);
      break;
    case mojom::ContributionProcessor::GEMINI:
      ledger().gemini()->StartContribution(contribution_id, std::move(info),
                                           amount, start_callback);
      break;
    default:
      NOTREACHED();
      BLOG(0, "Contribution processor not supported");
      break;
  }
}

void ContributionExternalWallet::Completed(mojom::Result result,
                                           bool single_publisher,
                                           LegacyResultCallback callback) {
  if (single_publisher) {
    callback(result);
    return;
  }

  callback(mojom::Result::RETRY);
}

void ContributionExternalWallet::Retry(mojom::ContributionInfoPtr contribution,
                                       LegacyResultCallback callback) {
  Process(contribution->contribution_id, callback);
}

}  // namespace brave_rewards::internal::contribution
