/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_EXTERNAL_WALLET_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_EXTERNAL_WALLET_H_

#include <map>
#include <string>

#include "brave/components/brave_rewards/core/ledger.h"

namespace brave_rewards::core {
class LedgerImpl;

namespace contribution {

class ContributionExternalWallet {
 public:
  explicit ContributionExternalWallet(LedgerImpl* ledger);

  ~ContributionExternalWallet();

  void Process(const std::string& contribution_id,
               LegacyResultCallback callback);

  void Retry(mojom::ContributionInfoPtr contribution,
             LegacyResultCallback callback);

 private:
  void ContributionInfo(mojom::ContributionInfoPtr contribution,
                        LegacyResultCallback callback);

  void OnAC(const mojom::Result result, const std::string& contribution_id);

  void OnSavePendingContribution(const mojom::Result result);

  void OnServerPublisherInfo(mojom::ServerPublisherInfoPtr info,
                             const std::string& contribution_id,
                             double amount,
                             mojom::RewardsType type,
                             mojom::ContributionProcessor processor,
                             bool single_publisher,
                             LegacyResultCallback callback);

  void Completed(mojom::Result result,
                 bool single_publisher,
                 LegacyResultCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace contribution
}  // namespace brave_rewards::core
#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_EXTERNAL_WALLET_H_
