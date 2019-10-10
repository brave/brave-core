/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_PRIVATE_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_PRIVATE_OBSERVER_H_

#include "base/observer_list_types.h"
#include "bat/ledger/mojom_structs.h"

namespace brave_rewards {

class RewardsService;
struct BalanceReport;

class RewardsServicePrivateObserver : public base::CheckedObserver {
 public:
  ~RewardsServicePrivateObserver() override {}

  virtual void OnGetCurrentBalanceReport(RewardsService* rewards_service,
                                         const BalanceReport& balance_report) {}
  virtual void OnPanelPublisherInfo(
      RewardsService* rewards_service,
      int error_code,
      const ledger::PublisherInfo* info,
      uint64_t windowId) {}
  virtual void OnRewardsMainEnabled(
      RewardsService* rewards_service,
      bool rewards_main_enabled) {}
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_PRIVATE_OBSERVER_H_
