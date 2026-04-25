/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_ADDRESS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_ADDRESS_H_

#include <string>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

inline constexpr size_t kEthAddressLength = 20u;
inline constexpr size_t kEthPublicKeyLength = 64u;

// Special address that presents contract creation.
class EthContractCreationAddress {
 public:
  EthContractCreationAddress();
  ~EthContractCreationAddress();

  std::string ToHex() const { return "0x"; }

  base::span<const uint8_t> bytes() const { return base::span<uint8_t>(); }

  bool operator==(const EthContractCreationAddress&) const;
};

class EthAddress {
 public:
  EthAddress(const EthAddress& other);
  ~EthAddress();
  bool operator==(const EthAddress& other) const;

  // public key must be uncompressed and no header byte so its length is 64
  // bytes
  static EthAddress FromPublicKey(
      base::span<const uint8_t, kEthPublicKeyLength> public_key);
  static EthAddress FromBytes(
      base::span<const uint8_t, kEthAddressLength> bytes);

  // input should be a valid address with 20 bytes hex representation starting
  // with 0x
  static std::optional<EthAddress> From0xHex(std::string_view input);
  static EthAddress ZeroAddress();
  static bool IsValidAddress(std::string_view input);
  static std::optional<std::string> ToEip1191ChecksumAddress(
      std::string_view address,
      std::string_view chain_id);

  bool IsZeroAddress() const;
  base::span<const uint8_t, kEthAddressLength> bytes() const { return bytes_; }

  std::string ToHex() const;
  // EIP55 + EIP1191
  std::string ToChecksumAddress(uint256_t eip1191_chaincode = 0) const;

 private:
  EthAddress();
  explicit EthAddress(base::span<const uint8_t, kEthAddressLength> bytes);

  std::array<uint8_t, kEthAddressLength> bytes_ = {};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_ADDRESS_H_
