/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_WALLET_WALLET_H_
#define BRAVELEDGER_WALLET_WALLET_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>

#include "bat/ledger/internal/wallet/wallet_balance.h"
#include "bat/ledger/internal/wallet/wallet_claim.h"
#include "bat/ledger/internal/wallet/wallet_create.h"
#include "bat/ledger/internal/wallet/wallet_recover.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace wallet {

class Wallet {
 public:
  explicit Wallet(LedgerImpl* ledger);
  ~Wallet();

  void CreateWalletIfNecessary(ledger::ResultCallback callback);

  void RecoverWallet(
      const std::string& pass_phrase,
      ledger::ResultCallback callback);

  std::string GetWalletPassphrase(type::BraveWalletPtr wallet);

  void FetchBalance(ledger::FetchBalanceCallback callback);

  void ExternalWalletAuthorization(
      const std::string& wallet_type,
      const std::map<std::string, std::string>& args,
      ledger::ExternalWalletAuthorizationCallback callback);

  void DisconnectWallet(
      const std::string& wallet_type,
      ledger::ResultCallback callback);

  void ClaimFunds(ledger::ResultCallback callback);

  void GetAnonWalletStatus(ledger::ResultCallback callback);

  void DisconnectAllWallets(ledger::ResultCallback callback);

  type::BraveWalletPtr GetWallet();

  bool SetWallet(type::BraveWalletPtr wallet);

 private:
  void AuthorizeWallet(
      const std::string& wallet_type,
      const std::map<std::string, std::string>& args,
      ledger::ExternalWalletAuthorizationCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<WalletCreate> create_;
  std::unique_ptr<WalletRecover> recover_;
  std::unique_ptr<WalletBalance> balance_;
  std::unique_ptr<WalletClaim> claim_;
};

}  // namespace wallet
}  // namespace ledger
#endif  // BRAVELEDGER_WALLET_WALLET_H_
