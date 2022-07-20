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
             ledger::LegacyResultCallback callback) override;

  void RedeemTokens(const CredentialsRedeem& redeem,
                    ledger::LegacyResultCallback callback) override;

 private:
  void OnStart(type::CredsBatchPtr creds,
               const CredentialsTrigger& trigger,
               ledger::LegacyResultCallback callback);

  void Blind(const CredentialsTrigger& trigger,
             ledger::LegacyResultCallback callback) override;

  void OnBlind(type::Result result,
               const CredentialsTrigger& trigger,
               ledger::LegacyResultCallback callback);

  void RetryPreviousStepSaved(type::Result result,
                              ledger::LegacyResultCallback callback);

  void Claim(type::CredsBatchPtr creds,
             const CredentialsTrigger& trigger,
             ledger::LegacyResultCallback callback) override;

  void OnClaim(type::Result result,
               const CredentialsTrigger& trigger,
               ledger::LegacyResultCallback callback);

  void ClaimStatusSaved(type::Result result,
                        const CredentialsTrigger& trigger,
                        ledger::LegacyResultCallback callback);

  void FetchSignedCreds(const CredentialsTrigger& trigger,
                        ledger::LegacyResultCallback callback);

  void OnFetchSignedCreds(type::Result result,
                          type::CredsBatchPtr batch,
                          const CredentialsTrigger& trigger,
                          ledger::LegacyResultCallback callback);

  void SignedCredsSaved(type::Result result,
                        const CredentialsTrigger& trigger,
                        ledger::LegacyResultCallback callback);

  void Unblind(type::CredsBatchPtr creds,
               const CredentialsTrigger& trigger,
               ledger::LegacyResultCallback callback) override;

  void Completed(type::Result result,
                 const CredentialsTrigger& trigger,
                 ledger::LegacyResultCallback callback) override;

  void OnRedeemTokens(type::Result result,
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
