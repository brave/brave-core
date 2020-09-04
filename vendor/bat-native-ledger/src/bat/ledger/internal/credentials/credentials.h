/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CREDENTIALS_H_
#define BRAVELEDGER_CREDENTIALS_H_

#include <map>
#include <string>

#include "bat/ledger/mojom_structs.h"
#include "bat/ledger/internal/credentials/credentials_redeem.h"
#include "bat/ledger/internal/credentials/credentials_trigger.h"

namespace ledger {
namespace credential {

class Credentials {
 public:
  virtual ~Credentials() = default;

  virtual void Start(
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) = 0;

  virtual void RedeemTokens(
      const CredentialsRedeem& redeem,
      ledger::ResultCallback callback) = 0;

 protected:
  virtual void Blind(
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) = 0;

  virtual void Claim(
      type::CredsBatchPtr creds,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) = 0;

  virtual void Unblind(
      type::CredsBatchPtr creds,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) = 0;

  virtual void Completed(
      const type::Result result,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) = 0;
};

}  // namespace credential
}  // namespace ledger

#endif  // BRAVELEDGER_CREDENTIALS_H_
