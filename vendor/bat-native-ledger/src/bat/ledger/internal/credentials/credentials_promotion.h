/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CREDENTIALS_PROMOTION_H_
#define BRAVELEDGER_CREDENTIALS_PROMOTION_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/internal/credentials/credentials_common.h"
#include "bat/ledger/internal/endpoint/promotion/promotion_server.h"

namespace braveledger_credentials {

class CredentialsPromotion : public Credentials {
 public:
  explicit CredentialsPromotion(bat_ledger::LedgerImpl* ledger);
  ~CredentialsPromotion() override;

  void Start(
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) override;

  void RedeemTokens(
      const CredentialsRedeem& redeem,
      ledger::ResultCallback callback) override;

 private:
  void OnStart(
      ledger::CredsBatchPtr creds,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback);

  void Blind(
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) override;

  void OnBlind(
    const ledger::Result result,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback);

  void Claim(
      ledger::CredsBatchPtr creds,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) override;

  void OnClaim(
      const ledger::Result result,
      const std::string& claim_id,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback);

  void ClaimedSaved(
      const ledger::Result result,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback);

  void ClaimStatusSaved(
      const ledger::Result result,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback);

  void RetryPreviousStepSaved(
      const ledger::Result result,
      ledger::ResultCallback callback);

  void FetchSignedCreds(
      ledger::PromotionPtr promotion,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback);

  void OnFetchSignedCreds(
      const ledger::Result result,
      ledger::CredsBatchPtr batch,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback);

  void SignedCredsSaved(
      const ledger::Result result,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback);

  void Unblind(
      ledger::CredsBatchPtr creds,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) override;

  void VerifyPublicKey(
      ledger::PromotionPtr promotion,
      const CredentialsTrigger& trigger,
      const ledger::CredsBatch& creds,
      ledger::ResultCallback callback);

  void SaveUnblindedCreds(
      ledger::PromotionPtr promotion,
      const ledger::CredsBatch& creds,
      const std::vector<std::string>& unblinded_encoded_creds,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback);

  void Completed(
      const ledger::Result result,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) override;

  void OnRedeemTokens(
      const ledger::Result result,
      const std::vector<std::string>& token_id_list,
      const CredentialsRedeem& redeem,
      ledger::ResultCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<CredentialsCommon> common_;
  std::unique_ptr<ledger::endpoint::PromotionServer> promotion_server_;
};

}  // namespace braveledger_credentials

#endif  // BRAVELEDGER_CREDENTIALS_PROMOTION_H_
