/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CREDENTIALS_CREDENTIALS_SKU_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CREDENTIALS_CREDENTIALS_SKU_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/internal/credentials/credentials_common.h"
#include "bat/ledger/internal/endpoint/payment/payment_server.h"

namespace ledger {
namespace credential {

class CredentialsSKU : public Credentials {
 public:
  explicit CredentialsSKU(LedgerImpl* ledger);
  ~CredentialsSKU() override;

  void Start(const CredentialsTrigger& trigger,
             ledger::ResultCallback callback) override;

  void RedeemTokens(const CredentialsRedeem& redeem,
                    ledger::LegacyResultCallback callback) override;

 private:
  void OnStart(ledger::ResultCallback callback,
               const CredentialsTrigger& trigger,
               mojom::CredsBatchPtr creds);

  void Blind(ledger::ResultCallback callback,
             const CredentialsTrigger& trigger) override;

  void OnBlind(ledger::ResultCallback callback,
               const CredentialsTrigger& trigger,
               mojom::Result result);

  void RetryPreviousStepSaved(ledger::ResultCallback callback,
                              mojom::Result result);

  void Claim(ledger::ResultCallback callback,
             const CredentialsTrigger& trigger,
             mojom::CredsBatchPtr creds) override;

  void OnClaim(ledger::ResultCallback callback,
               const CredentialsTrigger& trigger,
               mojom::Result result);

  void ClaimStatusSaved(ledger::ResultCallback callback,
                        const CredentialsTrigger& trigger,
                        mojom::Result result);

  void FetchSignedCreds(ledger::ResultCallback callback,
                        const CredentialsTrigger& trigger);

  void OnFetchSignedCreds(ledger::ResultCallback callback,
                          const CredentialsTrigger& trigger,
                          mojom::Result result,
                          mojom::CredsBatchPtr batch);

  void SignedCredsSaved(ledger::ResultCallback callback,
                        const CredentialsTrigger& trigger,
                        mojom::Result result);

  void Unblind(ledger::ResultCallback callback,
               const CredentialsTrigger& trigger,
               mojom::CredsBatchPtr creds) override;

  void Completed(ledger::ResultCallback callback,
                 const CredentialsTrigger& trigger,
                 mojom::Result result) override;

  void OnRedeemTokens(mojom::Result result,
                      const std::vector<std::string>& token_id_list,
                      const CredentialsRedeem& redeem,
                      ledger::LegacyResultCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<CredentialsCommon> common_;
  std::unique_ptr<endpoint::PaymentServer> payment_server_;
};

}  // namespace credential
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CREDENTIALS_CREDENTIALS_SKU_H_
