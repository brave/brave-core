/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PROMOTION_TRANSFER_H_
#define BRAVELEDGER_PROMOTION_TRANSFER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/internal/credentials/credentials_factory.h"
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

  void OnGetEligibleTokens(
      ledger::UnblindedTokenList list,
      ledger::ResultCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<braveledger_credentials::Credentials> credentials_;
};

}  // namespace braveledger_promotion

#endif  // BRAVELEDGER_PROMOTION_TRANSFER_H_
