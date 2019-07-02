/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_UPHOLD_UPHOLD_CARD_H_
#define BRAVELEDGER_UPHOLD_UPHOLD_CARD_H_

#include <map>
#include <string>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/uphold/uphold.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_uphold {

class UpholdCard {
 public:
  explicit UpholdCard(bat_ledger::LedgerImpl* ledger);

  ~UpholdCard();

  void Create(
    ledger::ExternalWalletPtr wallet,
    CreateCardCallback callback);

 private:
  void OnCreate(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      CreateCardCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_uphold
#endif  // BRAVELEDGER_UPHOLD_UPHOLD_CARD_H_
