/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_WALLET_RECOVER_H_
#define BRAVELEDGER_WALLET_RECOVER_H_

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

class Recover {
 public:
  explicit Recover(bat_ledger::LedgerImpl* ledger);

  ~Recover();

  void Start(
      const std::string& pass_phrase,
      ledger::RecoverWalletCallback callback);

 private:
  void OnNicewareListLoaded(
      const std::string& pass_phrase,
      ledger::Result result,
      const std::string& data,
      ledger::RecoverWalletCallback callback);

  void ContinueRecover(
      int result,
      size_t* written,
      const std::vector<uint8_t>& newSeed,
      ledger::RecoverWalletCallback callback);


  void RecoverWalletPublicKeyCallback(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      const std::vector<uint8_t>& new_seed,
      ledger::RecoverWalletCallback callback);

  void RecoverWalletCallback(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      const std::string& recoveryId,
      const std::vector<uint8_t>& new_seed,
      ledger::RecoverWalletCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_wallet
#endif  // BRAVELEDGER_WALLET_RECOVER_H_
