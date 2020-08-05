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

#include "base/timer/timer.h"
#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_contribution {

class Unverified {
 public:
  explicit Unverified(bat_ledger::LedgerImpl* ledger);

  ~Unverified();

  void Contribute();

 private:
  void WasPublisherProcessed(
      const ledger::Result result,
      const std::string& publisher_key,
      const std::string& name);

  void ProcessedPublisherSaved(
      const ledger::Result result,
      const std::string& publisher_key,
      const std::string& name);

  void OnRemovePendingContribution(ledger::Result result);

  void OnContributeUnverifiedBalance(
      ledger::Result result,
      ledger::BalancePtr properties);

  void OnContributeUnverifiedPublishers(
      double balance,
      const ledger::PendingContributionInfoList& list);

  void QueueSaved(
      const ledger::Result result,
      const uint64_t pending_contribution_id);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  base::OneShotTimer unverified_publishers_timer_;
};

}  // namespace braveledger_contribution
#endif  // BRAVELEDGER_CONTRIBUTION_UNVERIFIED_H_
