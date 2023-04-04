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

#include "base/timer/timer.h"
#include "brave/components/brave_rewards/core/attestation/attestation_impl.h"
#include "brave/components/brave_rewards/core/credentials/credentials_factory.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/promotion_server.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"
#include "brave/components/brave_rewards/core/mojom_structs.h"

namespace ledger {
class LedgerImpl;

namespace promotion {

class PromotionTransfer;

class Promotion {
 public:
  explicit Promotion(LedgerImpl* ledger);
  ~Promotion();

  void Initialize();

  void Fetch(ledger::FetchPromotionsCallback callback);

  void Claim(const std::string& promotion_id,
             const std::string& payload,
             ledger::ClaimPromotionCallback callback);

  void Attest(const std::string& promotion_id,
              const std::string& solution,
              ledger::AttestPromotionCallback callback);

  void Refresh(const bool retry_after_error);

  void TransferTokens(ledger::PostSuggestionsClaimCallback callback);

 private:
  void OnFetch(ledger::FetchPromotionsCallback callback,
               mojom::Result result,
               std::vector<mojom::PromotionPtr> list,
               const std::vector<std::string>& corrupted_promotions);

  void OnGetAllPromotions(
      ledger::FetchPromotionsCallback callback,
      std::vector<mojom::PromotionPtr> list,
      base::flat_map<std::string, mojom::PromotionPtr> promotions);

  void OnGetAllPromotionsFromDatabase(
      ledger::FetchPromotionsCallback callback,
      base::flat_map<std::string, mojom::PromotionPtr> promotions);

  void LegacyClaimedSaved(
      const mojom::Result result,
      std::shared_ptr<mojom::PromotionPtr> shared_promotion);

  void OnClaimPromotion(ledger::ClaimPromotionCallback callback,
                        const std::string& payload,
                        mojom::PromotionPtr promotion);

  void OnAttestPromotion(ledger::AttestPromotionCallback callback,
                         const std::string& solution,
                         mojom::PromotionPtr promotion);

  void OnAttestedPromotion(ledger::AttestPromotionCallback callback,
                           const std::string& promotion_id,
                           mojom::Result result);

  void OnCompletedAttestation(ledger::AttestPromotionCallback callback,
                              mojom::PromotionPtr promotion);

  void AttestedSaved(ledger::AttestPromotionCallback callback,
                     mojom::PromotionPtr promotion,
                     mojom::Result result);

  void Complete(ledger::AttestPromotionCallback callback,
                const std::string& promotion_string,
                mojom::Result result);

  void OnComplete(ledger::AttestPromotionCallback callback,
                  mojom::Result result,
                  mojom::PromotionPtr promotion);

  void ProcessFetchedPromotions(const mojom::Result result,
                                std::vector<mojom::PromotionPtr> promotions,
                                ledger::FetchPromotionsCallback callback);

  void GetCredentials(ledger::ResultCallback callback,
                      mojom::PromotionPtr promotion);

  void CredentialsProcessed(ledger::ResultCallback callback,
                            const std::string& promotion_id,
                            mojom::Result result);

  void Retry(base::flat_map<std::string, mojom::PromotionPtr> promotions);

  void CheckForCorrupted(
      const base::flat_map<std::string, mojom::PromotionPtr>& promotions);

  void CorruptedPromotionFixed(const mojom::Result result);

  void CheckForCorruptedCreds(std::vector<mojom::CredsBatchPtr> list);

  void CorruptedPromotions(std::vector<mojom::PromotionPtr> promotions,
                           const std::vector<std::string>& ids);

  void OnCheckForCorrupted(const mojom::Result result,
                           const std::vector<std::string>& promotion_id_list);

  void ErrorStatusSaved(const mojom::Result result,
                        const std::vector<std::string>& promotion_id_list);

  void ErrorCredsStatusSaved(const mojom::Result result);

  void OnRetryTimerElapsed();

  void OnLastCheckTimerElapsed();

  std::unique_ptr<ledger::attestation::AttestationImpl> attestation_;
  std::unique_ptr<PromotionTransfer> transfer_;
  std::unique_ptr<credential::Credentials> credentials_;
  std::unique_ptr<endpoint::PromotionServer> promotion_server_;
  LedgerImpl* ledger_;  // NOT OWNED
  base::OneShotTimer last_check_timer_;
  base::OneShotTimer retry_timer_;
};

}  // namespace promotion
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PROMOTION_PROMOTION_H_
