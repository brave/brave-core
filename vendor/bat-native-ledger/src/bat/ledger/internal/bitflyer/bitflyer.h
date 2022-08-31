/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/containers/flat_map.h"
#include "base/timer/timer.h"
#include "bat/ledger/internal/wallet_provider/wallet_provider.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace endpoint {
class BitflyerServer;
}

namespace bitflyer {

struct Transaction {
  std::string address;
  double amount;
  std::string message;
};

class BitflyerTransfer;
class BitflyerAuthorization;
class BitflyerWallet;

using FetchBalanceCallback = base::OnceCallback<void(type::Result, double)>;

class Bitflyer : public WalletProvider {
 public:
  explicit Bitflyer(LedgerImpl* ledger);

  ~Bitflyer() override;

  const char* Name() const override;

  type::ExternalWalletPtr GenerateLinks(
      type::ExternalWalletPtr wallet) override;

  void Initialize();

  void StartContribution(const std::string& contribution_id,
                         type::ServerPublisherInfoPtr info,
                         double amount,
                         ledger::LegacyResultCallback callback);

  void FetchBalance(FetchBalanceCallback callback);

  void TransferFunds(const double amount,
                     const std::string& address,
                     client::TransactionCallback callback);

  void WalletAuthorization(
      const base::flat_map<std::string, std::string>& args,
      ledger::ExternalWalletAuthorizationCallback callback);

  void GenerateWallet(ledger::ResultCallback callback);

 private:
  void ContributionCompleted(type::Result result,
                             const std::string& transaction_id,
                             const std::string& contribution_id,
                             double fee,
                             const std::string& publisher_key,
                             ledger::LegacyResultCallback callback);

  void OnFetchBalance(FetchBalanceCallback callback,
                      const type::Result result,
                      const double available);

  void SaveTransferFee(const std::string& contribution_id, const double amount);

  void StartTransferFeeTimer(const std::string& fee_id, const int attempts);

  void OnTransferFeeCompleted(const type::Result result,
                              const std::string& transaction_id,
                              const std::string& contribution_id,
                              const int attempts);

  void TransferFee(const std::string& contribution_id,
                   const double amount,
                   const int attempts);

  void OnTransferFeeTimerElapsed(const std::string& id, const int attempts);

  void RemoveTransferFee(const std::string& contribution_id);

  std::unique_ptr<BitflyerTransfer> transfer_;
  std::unique_ptr<BitflyerAuthorization> authorization_;
  std::unique_ptr<BitflyerWallet> wallet_;
  std::unique_ptr<endpoint::BitflyerServer> bitflyer_server_;
  LedgerImpl* ledger_;  // NOT OWNED
  std::map<std::string, base::OneShotTimer> transfer_fee_timers_;
};

}  // namespace bitflyer
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_H_
