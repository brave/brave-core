/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_KEYRING_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/hd_keyring.h"

namespace brave_wallet {

class EthereumKeyring : public HDKeyring {
 public:
  EthereumKeyring() = default;
  ~EthereumKeyring() override = default;
  EthereumKeyring(const EthereumKeyring&) = delete;
  EthereumKeyring& operator=(const EthereumKeyring&) = delete;

  // Obtains the address that signed the message
  // message: The keccak256 hash of the message (33 bytes = 04 prefix + 32
  // bytes)
  // signature: The 64 byte signature + v parameter (0 chain id assumed)
  static bool RecoverAddress(const std::vector<uint8_t>& message,
                             const std::vector<uint8_t>& signature,
                             std::string* address);

  std::vector<uint8_t> SignMessage(const std::string& address,
                                   const std::vector<uint8_t>& message,
                                   uint256_t chain_id,
                                   bool is_eip712);

  void SignTransaction(const std::string& address,
                       EthTransaction* tx,
                       uint256_t chain_id);

  bool GetPublicKeyFromX25519_XSalsa20_Poly1305(const std::string& address,
                                                std::string* key);

 private:
  std::string GetAddressInternal(HDKeyBase* hd_key) const override;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_KEYRING_H_
