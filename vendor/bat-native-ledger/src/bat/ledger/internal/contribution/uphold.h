/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CONTRIBUTION_UPHOLD_H_
#define BRAVELEDGER_CONTRIBUTION_UPHOLD_H_

#include <string>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_contribution {

class Uphold {
 public:
  explicit Uphold(bat_ledger::LedgerImpl* ledger);

  ~Uphold();
  void Start(const std::string &viewing_id, const std::string& token);

 private:

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_contribution
#endif  // BRAVELEDGER_CONTRIBUTION_UPHOLD_H_
