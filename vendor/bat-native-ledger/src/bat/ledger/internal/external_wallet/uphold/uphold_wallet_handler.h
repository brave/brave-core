/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_EXTERNAL_WALLET_UPHOLD_UPHOLD_WALLET_HANDLER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_EXTERNAL_WALLET_UPHOLD_UPHOLD_WALLET_HANDLER_H_

#include <string>

#include "bat/ledger/internal/core/bat_ledger_context.h"
#include "bat/ledger/internal/core/future.h"
#include "bat/ledger/internal/external_wallet/external_wallet_handler.h"

namespace ledger {

class UpholdWalletHandler : public BATLedgerContext::Object,
                            public ExternalWalletHandler {
 public:
  inline static const char kContextKey[] = "uphold-wallet-handler";

  Future<absl::optional<double>> GetBalance(
      const ExternalWallet& wallet) override;

  Future<absl::optional<std::string>> TransferBAT(
      const ExternalWallet& wallet,
      const std::string& destination,
      double amount,
      const std::string& description) override;

  std::string GetContributionFeeAddress() override;

  absl::optional<std::string> GetContributionTokenOrderAddress() override;
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_EXTERNAL_WALLET_UPHOLD_UPHOLD_WALLET_HANDLER_H_
