/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_EXTERNAL_WALLET_EXTERNAL_WALLET_HANDLER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_EXTERNAL_WALLET_EXTERNAL_WALLET_HANDLER_H_

#include <string>

#include "bat/ledger/internal/core/future.h"
#include "bat/ledger/internal/external_wallet/external_wallet_data.h"

namespace ledger {

class ExternalWalletHandler {
 public:
  ExternalWalletHandler();
  virtual ~ExternalWalletHandler();

  ExternalWalletHandler(const ExternalWalletHandler&) = delete;
  ExternalWalletHandler& operator=(const ExternalWalletHandler&) = delete;

  virtual Future<absl::optional<double>> GetBalance(
      const ExternalWallet& wallet) = 0;

  virtual Future<absl::optional<std::string>> TransferBAT(
      const ExternalWallet& wallet,
      const std::string& destination,
      double amount,
      const std::string& description) = 0;

  virtual std::string GetContributionFeeAddress() = 0;

  virtual absl::optional<std::string> GetContributionTokenOrderAddress() = 0;
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_EXTERNAL_WALLET_EXTERNAL_WALLET_HANDLER_H_
