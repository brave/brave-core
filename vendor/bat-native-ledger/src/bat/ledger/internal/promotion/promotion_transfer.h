/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PROMOTION_TRANSFER_H_
#define BRAVELEDGER_PROMOTION_TRANSFER_H_

#include <map>
#include <string>
#include <vector>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_promotion {

class PromotionTransfer {
 public:
  explicit PromotionTransfer(bat_ledger::LedgerImpl* ledger);
  ~PromotionTransfer();

  void Start(ledger::ExternalWalletPtr wallet, ledger::ResultCallback callback);

 private:
  void OnAnonExternalWallet(
      const ledger::Result result,
      ledger::ResultCallback callback);

  void GetEligiblePromotion(ledger::ResultCallback callback);

  void GetEligibleTokens(
      ledger::PromotionList promotions,
      ledger::ResultCallback callback);

  void SendTokens(
      ledger::UnblindedTokenList list,
      ledger::ResultCallback callback);

  void DeleteTokens(
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      const std::vector<std::string>& sent_ids,
      ledger::ResultCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_promotion

#endif  // BRAVELEDGER_PROMOTION_TRANSFER_H_
