/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_PRIVATE_OBSERVER_H_
#define BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_PRIVATE_OBSERVER_H_

#include "base/observer_list_types.h"
#include "bat/ledger/publisher_info.h"

namespace brave_rewards {

class RewardsService;
struct BalanceReport;

class RewardsServicePrivateObserver : public base::CheckedObserver {
 public:
  ~RewardsServicePrivateObserver() override {}

  virtual void OnGetCurrentBalanceReport(RewardsService* rewards_service,
                                         const BalanceReport& balance_report) {}
  virtual void OnGetPublisherActivityFromUrl(
      RewardsService* rewards_service,
      int error_code,
      std::unique_ptr<ledger::PublisherInfo> info,
      uint64_t windowId) {}
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_PRIVATE_OBSERVER_H_
