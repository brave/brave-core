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

namespace ledger {
namespace credential {

class CredentialsPromotion : public Credentials {
 public:
  explicit CredentialsPromotion(LedgerImpl* ledger);
  ~CredentialsPromotion() override;

  void Start(
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) override;

  void RedeemTokens(const CredentialsRedeem& redeem,
                    ledger::ResultCallback callback) override;

  void DrainTokens(const CredentialsRedeem& redeem,
                   ledger::PostSuggestionsClaimCallback callback);

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

  void Claim(
      type::CredsBatchPtr creds,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) override;

  void OnClaim(
      const type::Result result,
      const std::string& claim_id,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback);

  void ClaimedSaved(
      const type::Result result,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback);

  void ClaimStatusSaved(
      const type::Result result,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback);

  void RetryPreviousStepSaved(
      const type::Result result,
      ledger::ResultCallback callback);

  void FetchSignedCreds(
      type::PromotionPtr promotion,
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

  void VerifyPublicKey(
      type::PromotionPtr promotion,
      const CredentialsTrigger& trigger,
      const type::CredsBatch& creds,
      ledger::ResultCallback callback);

  void SaveUnblindedCreds(
      type::PromotionPtr promotion,
      const type::CredsBatch& creds,
      const std::vector<std::string>& unblinded_encoded_creds,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback);

  void Completed(
      const type::Result result,
      const CredentialsTrigger& trigger,
      ledger::ResultCallback callback) override;

  void OnRedeemTokens(const type::Result result,
                      const std::vector<std::string>& token_id_list,
                      const CredentialsRedeem& redeem,
                      ledger::ResultCallback callback);

  void OnDrainTokens(const type::Result result,
                     std::string drain_id,
                     const std::vector<std::string>& token_id_list,
                     const CredentialsRedeem& redeem,
                     ledger::PostSuggestionsClaimCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<CredentialsCommon> common_;
  std::unique_ptr<endpoint::PromotionServer> promotion_server_;
};

}  // namespace credential
}  // namespace ledger

#endif  // BRAVELEDGER_CREDENTIALS_PROMOTION_H_
