/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_AC_H_
#define BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_AC_H_

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_contribution {

class ContributionAC {
 public:
  explicit ContributionAC(bat_ledger::LedgerImpl* ledger);

  ~ContributionAC();

  void Process(const uint64_t reconcile_stamp);

 private:
  void PreparePublisherList(ledger::PublisherInfoList list);

  void QueueSaved(const ledger::Result result);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_contribution
#endif  // BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_AC_H_
