/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_WALLET_PROVIDER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_WALLET_PROVIDER_H_

#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

class WalletProvider {
 public:
  WalletProvider(LedgerImpl*);
  virtual ~WalletProvider();

  virtual const char* Name() const = 0;

  virtual type::ExternalWalletPtr GenerateLinks(type::ExternalWalletPtr wallet) = 0;

  type::ExternalWalletPtr GetWallet() const;
  bool SetWallet(type::ExternalWalletPtr);

  void DisconnectWallet(const absl::optional<std::string>& notification);

protected:
  virtual type::ExternalWalletPtr ResetWallet(type::ExternalWalletPtr);

 private:
  void LogWalletStatusChange(absl::optional<type::WalletStatus> from,
                             type::WalletStatus to) const;

  absl::optional<type::WalletStatus> previous_status;
  LedgerImpl* ledger_;
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_WALLET_PROVIDER_H_
