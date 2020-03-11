/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CREDENTIALS_H_
#define BRAVELEDGER_CREDENTIALS_H_

#include <map>
#include <string>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/credentials/credentials_trigger.h"

namespace braveledger_credentials {

class Credentials {
 public:
  virtual ~Credentials() = default;

  virtual void Start(
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) = 0;

  virtual void Blind(
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) = 0;

  virtual void Claim(
      const ledger::Result result,
      const std::string& blinded_creds_json,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) = 0;

  virtual void Unblind(
      ledger::CredsBatchPtr creds,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) = 0;

  virtual void Completed(
      const ledger::Result result,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) = 0;
};

}  // namespace braveledger_credentials

#endif  // BRAVELEDGER_CREDENTIALS_H_
