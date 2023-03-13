/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CREDENTIALS_CREDENTIALS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CREDENTIALS_CREDENTIALS_H_

#include "brave/components/brave_rewards/core/credentials/credentials_redeem.h"
#include "brave/components/brave_rewards/core/credentials/credentials_trigger.h"
#include "brave/components/brave_rewards/core/mojom_structs.h"

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

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CREDENTIALS_CREDENTIALS_H_
