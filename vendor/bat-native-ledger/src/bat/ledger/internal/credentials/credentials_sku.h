/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CREDENTIALS_SKU_H_
#define BRAVELEDGER_CREDENTIALS_SKU_H_

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

  void Start(
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) override;

  void RedeemTokens(
      const CredentialsRedeem& redeem,
      ledger::ResultCallback callback) override;

 private:
  void OnStart(
      type::CredsBatchPtr creds,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback);

  void Blind(
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) override;

  void OnBlind(
      const type::Result result,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback);

  void RetryPreviousStepSaved(
      const type::Result result,
      ledger::ResultCallback callback);

  void Claim(
      type::CredsBatchPtr creds,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) override;

  void OnClaim(
      const type::Result result,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback);

  void ClaimStatusSaved(
      const type::Result result,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback);

  void FetchSignedCreds(
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback);

  void OnFetchSignedCreds(
      const type::Result result,
      type::CredsBatchPtr batch,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback);

  void SignedCredsSaved(
      const type::Result result,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback);

  void Unblind(
      type::CredsBatchPtr creds,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) override;

  void Completed(
      const type::Result result,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) override;

  void OnRedeemTokens(
      const type::Result result,
      const std::vector<std::string>& token_id_list,
      const CredentialsRedeem& redeem,
      ledger::ResultCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<CredentialsCommon> common_;
  std::unique_ptr<endpoint::PaymentServer> payment_server_;
};

}  // namespace credential
}  // namespace ledger

#endif  // BRAVELEDGER_CREDENTIALS_SKU_H_
