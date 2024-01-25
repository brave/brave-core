/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CREDENTIALS_CREDENTIALS_PROMOTION_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CREDENTIALS_CREDENTIALS_PROMOTION_H_

#include <map>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/core/credentials/credentials_common.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/promotion_server.h"

namespace brave_rewards::internal {
namespace credential {

class CredentialsPromotion : public Credentials {
 public:
  explicit CredentialsPromotion(RewardsEngineImpl& engine);
  ~CredentialsPromotion() override;

  void Start(const CredentialsTrigger& trigger,
             ResultCallback callback) override;

  void RedeemTokens(const CredentialsRedeem& redeem,
                    ResultCallback callback) override;

  void DrainTokens(const CredentialsRedeem& redeem,
                   PostSuggestionsClaimCallback callback);

 private:
  void OnStart(ResultCallback callback,
               const CredentialsTrigger& trigger,
               mojom::CredsBatchPtr creds);

  void Blind(ResultCallback callback,
             const CredentialsTrigger& trigger) override;

  void OnBlind(ResultCallback callback,
               const CredentialsTrigger& trigger,
               mojom::Result result);

  void Claim(ResultCallback callback,
             const CredentialsTrigger& trigger,
             mojom::CredsBatchPtr creds) override;

  void OnClaim(ResultCallback callback,
               const CredentialsTrigger& trigger,
               mojom::Result result,
               const std::string& claim_id);

  void ClaimedSaved(ResultCallback callback,
                    const CredentialsTrigger& trigger,
                    mojom::Result result);

  void ClaimStatusSaved(ResultCallback callback,
                        const CredentialsTrigger& trigger,
                        mojom::Result result);

  void RetryPreviousStepSaved(ResultCallback callback, mojom::Result result);

  void FetchSignedCreds(ResultCallback callback,
                        const CredentialsTrigger& trigger,
                        mojom::PromotionPtr promotion);

  void OnFetchSignedCreds(ResultCallback callback,
                          const CredentialsTrigger& trigger,
                          mojom::Result result,
                          mojom::CredsBatchPtr batch);

  void SignedCredsSaved(ResultCallback callback,
                        const CredentialsTrigger& trigger,
                        mojom::Result result);

  void Unblind(ResultCallback callback,
               const CredentialsTrigger& trigger,
               mojom::CredsBatchPtr creds) override;

  void VerifyPublicKey(ResultCallback callback,
                       const CredentialsTrigger& trigger,
                       const mojom::CredsBatch& creds,
                       mojom::PromotionPtr promotion);

  void Completed(ResultCallback callback,
                 const CredentialsTrigger& trigger,
                 mojom::Result result) override;

  void OnRedeemTokens(std::vector<std::string> token_id_list,
                      CredentialsRedeem redeem,
                      ResultCallback callback,
                      mojom::Result result);

  void OnDrainTokens(PostSuggestionsClaimCallback callback,
                     const std::vector<std::string>& token_id_list,
                     const CredentialsRedeem& redeem,
                     mojom::Result result,
                     std::string drain_id);

  const raw_ref<RewardsEngineImpl> engine_;
  CredentialsCommon common_;
  endpoint::PromotionServer promotion_server_;
  base::WeakPtrFactory<CredentialsPromotion> weak_factory_{this};
};

}  // namespace credential
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CREDENTIALS_CREDENTIALS_PROMOTION_H_
