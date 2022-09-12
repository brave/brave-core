/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CREDENTIALS_CREDENTIALS_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CREDENTIALS_CREDENTIALS_H_

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

  virtual void Start(const CredentialsTrigger& trigger,
                     ledger::ResultCallback callback) = 0;

  virtual void RedeemTokens(const CredentialsRedeem& redeem,
                            ledger::LegacyResultCallback callback) = 0;

 protected:
  virtual void Blind(ledger::ResultCallback callback,
                     const CredentialsTrigger& trigger) = 0;

  virtual void Claim(ledger::ResultCallback callback,
                     const CredentialsTrigger& trigger,
                     mojom::CredsBatchPtr creds) = 0;

  virtual void Unblind(ledger::ResultCallback callback,
                       const CredentialsTrigger& trigger,
                       mojom::CredsBatchPtr creds) = 0;

  virtual void Completed(ledger::ResultCallback callback,
                         const CredentialsTrigger& trigger,
                         mojom::Result result) = 0;
};

}  // namespace credential
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CREDENTIALS_CREDENTIALS_H_
