/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_WALLET_CREATE_H_
#define BRAVELEDGER_WALLET_CREATE_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/bat_helper.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_wallet {

class Create {
 public:
  explicit Create(bat_ledger::LedgerImpl* ledger);

  ~Create();

  void Start();

 private:
  void RequestCredentialsCallback(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void RegisterPersonaCallback(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  std::string GetAnonizeProof(const std::string& registrarVK,
                              const std::string& id,
                              std::string* preFlight);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_wallet
#endif  // BRAVELEDGER_WALLET_CREATE_H_
