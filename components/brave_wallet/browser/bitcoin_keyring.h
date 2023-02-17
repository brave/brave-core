/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_KEYRING_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/hd_keyring.h"

namespace brave_wallet {

class BitcoinKeyring : public HDKeyring {
 public:
  BitcoinKeyring() = default;
  ~BitcoinKeyring() override = default;
  BitcoinKeyring(const BitcoinKeyring&) = delete;
  BitcoinKeyring& operator=(const BitcoinKeyring&) = delete;

  std::string GetReceivingAddress(uint32_t account_index,
                                  uint32_t receiving_index);
  std::string GetChangeAddress(uint32_t account_index, uint32_t change_index);

 private:
  std::string GetAddressInternal(HDKeyBase* hd_key_base) const override;
  std::unique_ptr<HDKeyBase> DeriveAccount(uint32_t index) const override;
  std::unique_ptr<HDKeyBase> DeriveReceivingKey(uint32_t account_index,
                                                uint32_t receiving_index);
  std::unique_ptr<HDKeyBase> DeriveChangeKey(uint32_t account_index,
                                             uint32_t change_index);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_KEYRING_H_
