/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CREDENTIALS_COMMON_H_
#define BRAVELEDGER_CREDENTIALS_COMMON_H_

#include <stdint.h>

#include <string>

#include "bat/ledger/internal/credentials/credentials.h"
#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_credentials {

using BlindedCredsCallback =
    std::function<void(const ledger::Result, const std::string&)>;

class CredentialsCommon {
 public:
  explicit CredentialsCommon(bat_ledger::LedgerImpl* ledger);
  ~CredentialsCommon();

  void GetBlindedCreds(
      const CredentialsTrigger& trigger,
      BlindedCredsCallback callback);

  void GetSignedCredsFromResponse(
      const CredentialsTrigger& trigger,
      const std::string& response,
      ledger::ResultCallback callback);

 private:
  void BlindedCredsSaved(
      const ledger::Result result,
      const std::string& blinded_creds_json,
      BlindedCredsCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_credentials

#endif  // BRAVELEDGER_CREDENTIALS_COMMON_H_
