/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PROMOTION_H_
#define BRAVELEDGER_PROMOTION_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/ledger.h"
#include "bat/ledger/mojom_structs.h"
#include "bat/ledger/internal/attestation/attestation_impl.h"
#include "bat/ledger/internal/credentials/credentials_factory.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_promotion {

class PromotionTransfer;

class Promotion {
 public:
  explicit Promotion(bat_ledger::LedgerImpl* ledger);
  ~Promotion();

  void Initialize();

  void Fetch(ledger::FetchPromotionCallback callback);

  void Claim(
      const std::string& payload,
      ledger::ClaimPromotionCallback callback);

  void Attest(
      const std::string& promotion_id,
      const std::string& solution,
      ledger::AttestPromotionCallback callback);

  void Refresh(const bool retry_after_error);

  void OnTimer(const uint32_t timer_id);

  void TransferTokens(
      ledger::ExternalWalletPtr wallet,
      ledger::ResultCallback callback);

 private:
  void OnFetch(
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      ledger::FetchPromotionCallback callback);

  void OnGetAllPromotions(
      ledger::PromotionMap promotions,
      const std::string& response,
      ledger::FetchPromotionCallback callback);

  void LegacyClaimedSaved(
      const ledger::Result result,
      const std::string& promotion_string);

  void OnAttestPromotion(
      const ledger::Result result,
      const std::string& promotion_id,
      ledger::AttestPromotionCallback callback);

  void OnCompletedAttestation(
      ledger::PromotionPtr promotion,
      ledger::AttestPromotionCallback callback);

  void AttestedSaved(
      const ledger::Result result,
      const std::string& promotion_string,
      ledger::AttestPromotionCallback callback);

  void Complete(
      const ledger::Result result,
      const std::string& promotion_string,
      ledger::AttestPromotionCallback callback);

  void OnComplete(
      ledger::PromotionPtr promotion,
      const ledger::Result result,
      ledger::AttestPromotionCallback callback);

  void ProcessFetchedPromotions(
      const ledger::Result result,
      ledger::PromotionList promotions,
      ledger::FetchPromotionCallback callback);

  void GetCredentials(
      ledger::PromotionPtr promotion,
      ledger::ResultCallback callback);

  void CredentialsProcessed(
      const ledger::Result result,
      const std::string& promotion_id,
      ledger::ResultCallback callback);

  void Retry(ledger::PromotionMap promotions);

  void CheckForCorrupted(ledger::CredsBatchList list);

  void CorruptedPromotions(
      ledger::PromotionList promotions,
      const std::vector<std::string>& ids);

  void OnCheckForCorrupted(
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      const std::vector<std::string>& promotion_id_list);

  void PromotionListDeleted(const ledger::Result result);

  std::unique_ptr<braveledger_attestation::AttestationImpl> attestation_;
  std::unique_ptr<PromotionTransfer> transfer_;
  std::unique_ptr<braveledger_credentials::Credentials> credentials_;
  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  uint32_t last_check_timer_id_;
  uint32_t retry_timer_id_;
};

}  // namespace braveledger_promotion

#endif  // BRAVELEDGER_PROMOTION_H_
