/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_WALLET_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_WALLET_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "bat/ledger/internal/endpoint/promotion/promotion_server.h"
#include "bat/ledger/internal/wallet/wallet_balance.h"
#include "bat/ledger/internal/wallet/wallet_create.h"
#include "bat/ledger/ledger.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ledger {
class LedgerImpl;

namespace wallet {

class Wallet {
 public:
  explicit Wallet(LedgerImpl* ledger);
  ~Wallet();

  void CreateWalletIfNecessary(absl::optional<std::string>&& geo_country,
                               CreateRewardsWalletCallback callback);

  void FetchBalance(ledger::FetchBalanceCallback callback);

  mojom::RewardsWalletPtr GetWallet();
  mojom::RewardsWalletPtr GetWallet(bool* corrupted);

  bool SetWallet(mojom::RewardsWalletPtr wallet);

 private:
  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<WalletCreate> create_;
  std::unique_ptr<WalletBalance> balance_;
  std::unique_ptr<endpoint::PromotionServer> promotion_server_;
};

}  // namespace wallet
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_WALLET_H_
