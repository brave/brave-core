/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CONTRIBUTION_UNVERIFIED_H_
#define BRAVELEDGER_CONTRIBUTION_UNVERIFIED_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/contribution/contribution.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_contribution {

class Unverified {
 public:
  explicit Unverified(
      bat_ledger::LedgerImpl* ledger,
      Contribution* contribution);

  ~Unverified();
  void Contribute();

  void OnTimer(uint32_t timer_id);

 private:
  void OnRemovePendingContribution(ledger::Result result);

  void OnContributeUnverifiedBalance(
    ledger::Result result,
    ledger::BalancePtr properties);

  void OnContributeUnverifiedPublishers(
    double balance,
    const ledger::PendingContributionInfoList& list);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  Contribution* contribution_;   // NOT OWNED
  uint32_t unverified_publishers_timer_id_;
};

}  // namespace braveledger_contribution
#endif  // BRAVELEDGER_CONTRIBUTION_UNVERIFIED_H_
