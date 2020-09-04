/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CREDENTIALS_COMMON_H_
#define BRAVELEDGER_CREDENTIALS_COMMON_H_

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include "bat/ledger/internal/credentials/credentials.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace credential {

class CredentialsCommon {
 public:
  explicit CredentialsCommon(LedgerImpl* ledger);
  ~CredentialsCommon();

  void GetBlindedCreds(
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback);

  void SaveUnblindedCreds(
      const uint64_t expires_at,
      const double token_value,
      const type::CredsBatch& creds,
      const std::vector<std::string>& unblinded_encoded_creds,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback);

 private:
  void BlindedCredsSaved(
      const type::Result result,
      ledger::ResultCallback callback);

  void OnSaveUnblindedCreds(
      const type::Result result,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace credential
}  // namespace ledger

#endif  // BRAVELEDGER_CREDENTIALS_COMMON_H_
