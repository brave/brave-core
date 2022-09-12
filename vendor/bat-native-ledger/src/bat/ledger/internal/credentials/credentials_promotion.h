/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CREDENTIALS_CREDENTIALS_PROMOTION_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CREDENTIALS_CREDENTIALS_PROMOTION_H_

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

  void Start(const CredentialsTrigger& trigger,
             ledger::ResultCallback callback) override;

  void RedeemTokens(const CredentialsRedeem& redeem,
                    ledger::LegacyResultCallback callback) override;

  void DrainTokens(const CredentialsRedeem& redeem,
                   ledger::PostSuggestionsClaimCallback callback);

 private:
  void OnStart(ledger::ResultCallback callback,
               const CredentialsTrigger& trigger,
               mojom::CredsBatchPtr creds);

  void Blind(ledger::ResultCallback callback,
             const CredentialsTrigger& trigger) override;

  void OnBlind(ledger::ResultCallback callback,
               const CredentialsTrigger& trigger,
               mojom::Result result);

  void Claim(ledger::ResultCallback callback,
             const CredentialsTrigger& trigger,
             mojom::CredsBatchPtr creds) override;

  void OnClaim(ledger::ResultCallback callback,
               const CredentialsTrigger& trigger,
               mojom::Result result,
               const std::string& claim_id);

  void ClaimedSaved(ledger::ResultCallback callback,
                    const CredentialsTrigger& trigger,
                    mojom::Result result);

  void ClaimStatusSaved(ledger::ResultCallback callback,
                        const CredentialsTrigger& trigger,
                        mojom::Result result);

  void RetryPreviousStepSaved(ledger::ResultCallback callback,
                              mojom::Result result);

  void FetchSignedCreds(ledger::ResultCallback callback,
                        const CredentialsTrigger& trigger,
                        mojom::PromotionPtr promotion);

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

  void VerifyPublicKey(ledger::ResultCallback callback,
                       const CredentialsTrigger& trigger,
                       const mojom::CredsBatch& creds,
                       mojom::PromotionPtr promotion);

  void Completed(ledger::ResultCallback callback,
                 const CredentialsTrigger& trigger,
                 mojom::Result result) override;

  void OnRedeemTokens(mojom::Result result,
                      const std::vector<std::string>& token_id_list,
                      const CredentialsRedeem& redeem,
                      ledger::LegacyResultCallback callback);

  void OnDrainTokens(ledger::PostSuggestionsClaimCallback callback,
                     const std::vector<std::string>& token_id_list,
                     const CredentialsRedeem& redeem,
                     mojom::Result result,
                     std::string drain_id);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<CredentialsCommon> common_;
  std::unique_ptr<endpoint::PromotionServer> promotion_server_;
};

}  // namespace credential
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CREDENTIALS_CREDENTIALS_PROMOTION_H_
