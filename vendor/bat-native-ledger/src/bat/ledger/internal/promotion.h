/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PROMOTION_H_
#define BRAVELEDGER_PROMOTION_H_

#include <string>
#include <vector>
#include <map>

#include "bat/ledger/ledger.h"
#include "bat/ledger/mojom_structs.h"
#include "bat/ledger/internal/attestation/attestation_impl.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_promotion {

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

  void ClaimTokens(
      ledger::PromotionPtr promotion,
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

  void OnAttestPromotion(
      const ledger::Result result,
      const std::string& promotion_id,
      ledger::AttestPromotionCallback callback);

  void OnCompletedAttestation(
      ledger::PromotionPtr promotion,
      ledger::AttestPromotionCallback callback);

  void Complete(
      const ledger::Result result,
      const std::string& promotion_string,
      ledger::AttestPromotionCallback callback);

  void ProcessFetchedPromotions(
      const ledger::Result result,
      ledger::PromotionList promotions,
      ledger::FetchPromotionCallback callback);

  void Retry(ledger::PromotionMap promotions);

  void OnClaimTokens(
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      const std::string promotion_string,
      ledger::ResultCallback callback);

  void FetchSignedTokens(
      ledger::PromotionPtr promotion,
      ledger::ResultCallback callback);

  void OnFetchSignedTokens(
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      const std::string promotion_string,
      ledger::ResultCallback callback);

  void UnBlindTokens(
      ledger::PromotionPtr promotion,
      ledger::ResultCallback callback);

  std::unique_ptr<braveledger_attestation::AttestationImpl> attestation_;
  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  uint32_t last_check_timer_id_;
  uint32_t retry_timer_id_;
};

}  // namespace braveledger_promotion

#endif  // BRAVELEDGER_PROMOTION_H_
