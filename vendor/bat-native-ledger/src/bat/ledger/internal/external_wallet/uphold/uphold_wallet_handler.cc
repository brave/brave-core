/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/external_wallet/uphold/uphold_wallet_handler.h"

#include <string>

#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/core/environment_config.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/uphold/uphold_util.h"

namespace ledger {

namespace {

class FetchBalanceJob : public BATLedgerJob<absl::optional<double>> {
 public:
  void Start() {
    context().GetLedgerImpl()->uphold()->FetchBalance(
        ContinueWithLambda(this, &FetchBalanceJob::OnFetched));
  }

 private:
  void OnFetched(mojom::Result result, double balance) {
    if (result != mojom::Result::LEDGER_OK) {
      return Complete({});
    }

    Complete(balance);
  }
};

class TransferJob : public BATLedgerJob<absl::optional<std::string>> {
 public:
  void Start(const std::string& destination,
             double amount,
             const std::string& description) {
    context().GetLedgerImpl()->uphold()->TransferFunds(
        amount, destination, description,
        ContinueWithLambda(this, &TransferJob::OnCompleted));
  }

 private:
  void OnCompleted(mojom::Result result, const std::string& transaction_id) {
    if (result != mojom::Result::LEDGER_OK) {
      return Complete({});
    }

    Complete(transaction_id);
  }
};

}  // namespace

Future<absl::optional<double>> UpholdWalletHandler::GetBalance(
    const ExternalWallet& wallet) {
  return context().StartJob<FetchBalanceJob>();
}

Future<absl::optional<std::string>> UpholdWalletHandler::TransferBAT(
    const ExternalWallet& wallet,
    const std::string& destination,
    double amount,
    const std::string& description) {
  return context().StartJob<TransferJob>(destination, amount, description);
}

std::string UpholdWalletHandler::GetContributionFeeAddress() {
  return uphold::GetFeeAddress();
}

absl::optional<std::string>
UpholdWalletHandler::GetContributionTokenOrderAddress() {
  return context().Get<EnvironmentConfig>().uphold_token_order_address();
}

}  // namespace ledger
