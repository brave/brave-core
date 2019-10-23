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

  void Fetch(ledger::FetchPromotionCallback callback);

  void SetGrant(const std::string& captchaResponse,
                const std::string& promotionId,
                const std::string& safetynet_token);

  void ClaimPromotion(
      const std::string& payload,
      ledger::ClaimPromotionCallback callback);

  void Refresh(const bool retry_after_error);

  void OnTimer(const uint32_t timer_id);

 private:
  void OnFetch(
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      ledger::FetchPromotionCallback callback);

  void ProcessFetchedPromotions(
      const ledger::Result result,
      ledger::PromotionList promotions,
      ledger::FetchPromotionCallback callback);

  void SetGrantCallback(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      bool is_safetynet_check);

  std::unique_ptr<braveledger_attestation::AttestationImpl> attestation_;
  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  uint32_t last_check_timer_id_;
};

}  // namespace braveledger_promotion

#endif  // BRAVELEDGER_PROMOTION_H_
