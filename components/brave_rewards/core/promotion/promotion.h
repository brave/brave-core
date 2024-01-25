/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PROMOTION_PROMOTION_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PROMOTION_PROMOTION_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/core/attestation/attestation_impl.h"
#include "brave/components/brave_rewards/core/credentials/credentials_promotion.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/promotion_server.h"
#include "brave/components/brave_rewards/core/promotion/promotion_transfer.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace promotion {

class Promotion {
 public:
  explicit Promotion(RewardsEngineImpl& engine);
  ~Promotion();

  void Initialize();

  void Fetch(FetchPromotionsCallback callback);

  void Claim(const std::string& promotion_id,
             const std::string& payload,
             ClaimPromotionCallback callback);

  void Attest(const std::string& promotion_id,
              const std::string& solution,
              AttestPromotionCallback callback);

  void Refresh(const bool retry_after_error);

  void TransferTokens(PostSuggestionsClaimCallback callback);

 private:
  void OnFetch(FetchPromotionsCallback callback,
               mojom::Result result,
               std::vector<mojom::PromotionPtr> list,
               std::vector<std::string> corrupted_promotions);

  void OnGetAllPromotions(
      FetchPromotionsCallback callback,
      std::vector<mojom::PromotionPtr> list,
      base::flat_map<std::string, mojom::PromotionPtr> promotions);

  void OnGetAllPromotionsFromDatabase(
      FetchPromotionsCallback callback,
      base::flat_map<std::string, mojom::PromotionPtr> promotions);

  void LegacyClaimedSaved(mojom::PromotionPtr shared_promotion,
                          mojom::Result result);

  void OnClaimPromotion(ClaimPromotionCallback callback,
                        const std::string& payload,
                        mojom::PromotionPtr promotion);

  void OnAttestPromotion(AttestPromotionCallback callback,
                         const std::string& solution,
                         mojom::PromotionPtr promotion);

  void OnAttestedPromotion(AttestPromotionCallback callback,
                           const std::string& promotion_id,
                           mojom::Result result);

  void OnCompletedAttestation(AttestPromotionCallback callback,
                              mojom::PromotionPtr promotion);

  void AttestedSaved(AttestPromotionCallback callback,
                     mojom::PromotionPtr promotion,
                     mojom::Result result);

  void Complete(AttestPromotionCallback callback,
                const std::string& promotion_string,
                mojom::Result result);

  void OnComplete(AttestPromotionCallback callback,
                  mojom::Result result,
                  mojom::PromotionPtr promotion);

  void ProcessFetchedPromotions(const mojom::Result result,
                                std::vector<mojom::PromotionPtr> promotions,
                                FetchPromotionsCallback callback);

  void GetCredentials(ResultCallback callback, mojom::PromotionPtr promotion);

  void CredentialsProcessed(ResultCallback callback,
                            const std::string& promotion_id,
                            mojom::Result result);

  void Retry(base::flat_map<std::string, mojom::PromotionPtr> promotions);

  void CheckForCorrupted(
      base::flat_map<std::string, mojom::PromotionPtr> promotions);

  void CorruptedPromotionFixed(mojom::Result result);

  void CheckForCorruptedCreds(std::vector<mojom::CredsBatchPtr> list);

  void CorruptedPromotions(std::vector<std::string> ids,
                           std::vector<mojom::PromotionPtr> promotions);

  void OnCheckForCorrupted(std::vector<std::string> promotion_id_list,
                           mojom::Result result);

  void ErrorStatusSaved(std::vector<std::string> promotion_id_list,
                        mojom::Result result);

  void ErrorCredsStatusSaved(mojom::Result result);

  void OnRetryTimerElapsed();

  void OnLastCheckTimerElapsed();

  const raw_ref<RewardsEngineImpl> engine_;
  attestation::AttestationImpl attestation_;
  PromotionTransfer transfer_;
  credential::CredentialsPromotion credentials_;
  endpoint::PromotionServer promotion_server_;
  base::OneShotTimer last_check_timer_;
  base::OneShotTimer retry_timer_;
  base::WeakPtrFactory<Promotion> weak_factory_{this};
};

}  // namespace promotion
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PROMOTION_PROMOTION_H_
