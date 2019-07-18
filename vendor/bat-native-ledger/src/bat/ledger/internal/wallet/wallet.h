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
#include <vector>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/bat_helper.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_wallet {
class Create;
}

namespace braveledger_wallet {
class Recover;
}

namespace braveledger_wallet {
class Balance;
}

namespace braveledger_uphold {
class Uphold;
}

namespace braveledger_wallet {

class Wallet {
 public:
  explicit Wallet(bat_ledger::LedgerImpl* ledger);

  ~Wallet();

  void CreateWalletIfNecessary(const std::string& safetynet_token,
      ledger::CreateWalletCallback callback);

  void RecoverWallet(
      const std::string& pass_phrase,
      ledger::RecoverWalletCallback callback);

  std::string GetWalletPassphrase() const;

  void GetWalletProperties(ledger::OnWalletPropertiesCallback callback);

  ledger::WalletPropertiesPtr WalletPropertiesToWalletInfo(
    const braveledger_bat_helper::WALLET_PROPERTIES_ST& properties);

  void FetchBalance(ledger::FetchBalanceCallback callback);

  void GetExternalWallet(const std::string& wallet_type,
                         ledger::ExternalWalletCallback callback);

  void ExternalWalletAuthorization(
      const std::string& wallet_type,
      const std::map<std::string, std::string>& args,
      ledger::ExternalWalletAuthorizationCallback callback);

  void DisconnectWallet(
      const std::string& wallet_type,
      ledger::DisconnectWalletCallback callback);

  void TransferAnonToExternalWallet(
      ledger::ExternalWalletPtr wallet,
      const bool allow_zero_balance,
      ledger::TransferAnonToExternalWalletCallback callback);

  void GetGrantViaSafetynetCheck(const std::string& promotion_id);

 private:
  void WalletPropertiesCallback(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      ledger::OnWalletPropertiesCallback callback);

  void OnGetExternalWallet(
    const std::string& wallet_type,
    ledger::ExternalWalletCallback callback,
    std::map<std::string, ledger::ExternalWalletPtr> wallets);

  void OnExternalWalletAuthorization(
    const std::string& wallet_type,
    const std::map<std::string, std::string>& args,
    ledger::ExternalWalletAuthorizationCallback callback,
    std::map<std::string, ledger::ExternalWalletPtr> wallets);

  void OnDisconnectWallet(
    const std::string& wallet_type,
    ledger::DisconnectWalletCallback callback,
    std::map<std::string, ledger::ExternalWalletPtr> wallets);

  void OnTransferAnonToExternalWallet(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    ledger::TransferAnonToExternalWalletCallback callback);

  void OnTransferAnonToExternalWalletBalance(
      ledger::Result result,
      ledger::BalancePtr properties,
      const ledger::ExternalWallet& wallet,
      const bool allow_zero_balance,
      ledger::TransferAnonToExternalWalletCallback callback);

  std::string GetClaimPayload(
      const std::string user_funds,
      const std::string new_address,
      const std::string anon_address);

  void OnTransferAnonToExternalWalletAddress(
      ledger::Result result,
      const std::string& anon_address,
      const std::string& new_address,
      const std::string& user_funds,
      ledger::TransferAnonToExternalWalletCallback callback);

  void GetGrantViaSafetynetCheckCallback(const std::string& promotion_id,
                                         int response_status_code,
                                         const std::string& response,
                                         const std::map<std::string,
                                         std::string>& headers);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<Create> create_;
  std::unique_ptr<Recover> recover_;
  std::unique_ptr<Balance> balance_;
  std::unique_ptr<braveledger_uphold::Uphold> uphold_;
};

}  // namespace braveledger_wallet
#endif  // BRAVELEDGER_WALLET_WALLET_H_
