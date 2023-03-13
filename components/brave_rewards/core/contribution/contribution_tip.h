/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_TIP_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_TIP_H_

#include <string>

#include "brave/components/brave_rewards/core/ledger.h"

namespace ledger {
class LedgerImpl;

namespace contribution {

class ContributionTip {
 public:
  explicit ContributionTip(LedgerImpl* ledger);

  ~ContributionTip();

  void Process(const std::string& publisher_key,
               double amount,
               ledger::LegacyResultCallback callback);

 private:
  void ServerPublisher(mojom::ServerPublisherInfoPtr server_info,
                       const std::string& publisher_key,
                       double amount,
                       ledger::LegacyResultCallback callback);

  void QueueSaved(mojom::Result result, ledger::LegacyResultCallback callback);

  void SavePending(const std::string& publisher_key,
                   double amount,
                   ledger::LegacyResultCallback callback);

  void OnSavePending(mojom::Result result,
                     ledger::LegacyResultCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace contribution
}  // namespace ledger
#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_TIP_H_
