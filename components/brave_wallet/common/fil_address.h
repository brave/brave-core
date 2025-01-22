/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_FIL_ADDRESS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_FIL_ADDRESS_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class EthAddress;

class FilAddress {
 public:
  static std::optional<mojom::FilecoinAddressProtocol> GetProtocolFromAddress(
      const std::string& address);

  static FilAddress FromUncompressedPublicKey(
      const std::vector<uint8_t>& public_key,
      mojom::FilecoinAddressProtocol protocol,
      const std::string& network);

  static FilAddress FromPayload(const std::vector<uint8_t>& payload,
                                mojom::FilecoinAddressProtocol protocol,
                                const std::string& network);
  static FilAddress FromAddress(const std::string& address);
  static FilAddress FromBytes(const std::string& chain_id,
                              const std::vector<uint8_t>& bytes);
  static FilAddress FromFEVMAddress(bool is_mainnet,
                                    const EthAddress& fevm_address);

  static bool IsValidAddress(const std::string& input);
  FilAddress();
  FilAddress(const FilAddress& other);
  ~FilAddress();
  bool operator==(const FilAddress& other) const;
  bool operator!=(const FilAddress& other) const;

  bool IsEmpty() const { return bytes_.empty(); }
  mojom::FilecoinAddressProtocol protocol() const { return protocol_; }
  std::string EncodeAsString() const;
  std::string network() const { return network_; }
  // Represents byte form of the Filecoin address
  // https://spec.filecoin.io/appendix/address/#section-appendix.address.bytes
  std::vector<uint8_t> GetBytes() const;
  bool IsMainNet() const;

 private:
  bool IsEqual(const FilAddress& other) const;
  explicit FilAddress(const std::vector<uint8_t>& bytes,
                      mojom::FilecoinAddressProtocol protocol,
                      const std::string& network);

  mojom::FilecoinAddressProtocol protocol_ =
      mojom::FilecoinAddressProtocol::SECP256K1;
  std::string network_ = mojom::kFilecoinTestnet;
  std::vector<uint8_t> bytes_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_FIL_ADDRESS_H_
