/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_KEYRING_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/secp256k1_hd_keyring.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

class EthTransaction;

class EthereumKeyring : public Secp256k1HDKeyring {
 public:
  explicit EthereumKeyring(base::span<const uint8_t> seed);
  ~EthereumKeyring() override = default;
  EthereumKeyring(const EthereumKeyring&) = delete;
  EthereumKeyring& operator=(const EthereumKeyring&) = delete;

  // Obtains the address that signed the message
  // message: The keccak256 hash of the message (33 bytes = 04 prefix + 32
  // bytes)
  // signature: The 64 byte signature + v parameter (0 chain id assumed)
  static std::optional<std::string> RecoverAddress(
      base::span<const uint8_t> message,
      base::span<const uint8_t> signature);

  std::vector<uint8_t> SignMessage(const std::string& address,
                                   base::span<const uint8_t> message,
                                   uint256_t chain_id,
                                   bool is_eip712);

  void SignTransaction(const std::string& address,
                       EthTransaction* tx,
                       uint256_t chain_id);

  bool GetPublicKeyFromX25519_XSalsa20_Poly1305(const std::string& address,
                                                std::string* key);
  std::optional<std::vector<uint8_t>> DecryptCipherFromX25519_XSalsa20_Poly1305(
      const std::string& version,
      base::span<const uint8_t> nonce,
      base::span<const uint8_t> ephemeral_public_key,
      base::span<const uint8_t> ciphertext,
      const std::string& address);

  std::string EncodePrivateKeyForExport(const std::string& address) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(EthereumKeyringUnitTest, ConstructRootHDKey);

  std::string GetAddressInternal(const HDKey& hd_key) const override;
  std::unique_ptr<HDKey> DeriveAccount(uint32_t index) const override;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_KEYRING_H_
