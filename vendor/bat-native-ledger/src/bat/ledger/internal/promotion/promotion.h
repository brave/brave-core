/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PROMOTION_PROMOTION_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PROMOTION_PROMOTION_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/timer/timer.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/mojom_structs.h"
#include "bat/ledger/internal/attestation/attestation_impl.h"
#include "bat/ledger/internal/credentials/credentials_factory.h"
#include "bat/ledger/internal/endpoint/promotion/promotion_server.h"

namespace ledger {
class LedgerImpl;

namespace promotion {

class PromotionTransfer;

class Promotion {
 public:
  explicit Promotion(LedgerImpl* ledger);
  ~Promotion();

  void Initialize();

  void Fetch(ledger::FetchPromotionCallback callback);

  void Claim(
      const std::string& promotion_id,
      const std::string& payload,
      ledger::ClaimPromotionCallback callback);

  void Attest(
      const std::string& promotion_id,
      const std::string& solution,
      ledger::AttestPromotionCallback callback);

  void Refresh(const bool retry_after_error);

  void TransferTokens(ledger::PostSuggestionsClaimCallback callback);

  void GetTransferableAmount(ledger::GetTransferableAmountCallback callback);

  void GetDrainStatus(const std::string& drain_id,
                      ledger::GetDrainCallback callback);

 private:
  void OnFetch(
      const type::Result result,
      type::PromotionList list,
      const std::vector<std::string>& corrupted_promotions,
      ledger::FetchPromotionCallback callback);

  void OnGetAllPromotions(
      type::PromotionMap promotions,
      std::shared_ptr<type::PromotionList> list,
      ledger::FetchPromotionCallback callback);

  void OnGetAllPromotionsFromDatabase(
      type::PromotionMap promotions,
      ledger::FetchPromotionCallback callback);

  void LegacyClaimedSaved(
      const type::Result result,
      std::shared_ptr<type::PromotionPtr> shared_promotion);

  void OnClaimPromotion(
      type::PromotionPtr promotion,
      const std::string& payload,
      ledger::ClaimPromotionCallback callback);

  void OnAttestPromotion(
      type::PromotionPtr promotion,
      const std::string& solution,
      ledger::AttestPromotionCallback callback);

  void OnAttestedPromotion(
      const type::Result result,
      const std::string& promotion_id,
      ledger::AttestPromotionCallback callback);

  void OnCompletedAttestation(
      type::PromotionPtr promotion,
      ledger::AttestPromotionCallback callback);

  void AttestedSaved(
      const type::Result result,
      std::shared_ptr<type::PromotionPtr> shared_promotion,
      ledger::AttestPromotionCallback callback);

  void Complete(
      const type::Result result,
      const std::string& promotion_string,
      ledger::AttestPromotionCallback callback);

  void OnComplete(
      type::PromotionPtr promotion,
      const type::Result result,
      ledger::AttestPromotionCallback callback);

  void ProcessFetchedPromotions(
      const type::Result result,
      type::PromotionList promotions,
      ledger::FetchPromotionCallback callback);

  void GetCredentials(
      type::PromotionPtr promotion,
      ledger::ResultCallback callback);

  void CredentialsProcessed(
      const type::Result result,
      const std::string& promotion_id,
      ledger::ResultCallback callback);

  void Retry(type::PromotionMap promotions);

  void CheckForCorrupted(const type::PromotionMap& promotions);

  void CorruptedPromotionFixed(const type::Result result);

  void CheckForCorruptedCreds(type::CredsBatchList list);

  void CorruptedPromotions(
      type::PromotionList promotions,
      const std::vector<std::string>& ids);

  void OnCheckForCorrupted(
      const type::Result result,
      const std::vector<std::string>& promotion_id_list);

  void ErrorStatusSaved(
      const type::Result result,
      const std::vector<std::string>& promotion_id_list);

  void ErrorCredsStatusSaved(const type::Result result);

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

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PROMOTION_PROMOTION_H_
