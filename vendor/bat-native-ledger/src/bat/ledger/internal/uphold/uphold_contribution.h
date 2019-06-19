/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_UPHOLD_UPHOLD_CONTRIBUTION_H_
#define BRAVELEDGER_UPHOLD_UPHOLD_CONTRIBUTION_H_

#include <string>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_uphold {

class UpholdContribution {
 public:
  explicit UpholdContribution(bat_ledger::LedgerImpl* ledger);

  ~UpholdContribution();
  void Start(const std::string &viewing_id, ledger::ExternalWallet wallet);

 private:

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_uphold
#endif  // BRAVELEDGER_UPHOLD_UPHOLD_CONTRIBUTION_H_
