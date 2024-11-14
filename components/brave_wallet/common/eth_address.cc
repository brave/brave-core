/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/eth_address.h"

#include <utility>

#include "base/containers/span.h"
#include "base/logging.h"
#include "base/ranges/algorithm.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

EthAddress::EthAddress(std::vector<uint8_t> bytes) : bytes_(std::move(bytes)) {}
EthAddress::EthAddress(base::span<const uint8_t> bytes)
    : bytes_(bytes.begin(), bytes.end()) {}
EthAddress::EthAddress() = default;
EthAddress::EthAddress(const EthAddress& other) = default;
EthAddress::~EthAddress() = default;

bool EthAddress::operator==(const EthAddress& other) const {
  return bytes_ == other.bytes_;
}

bool EthAddress::operator!=(const EthAddress& other) const {
  return !(*this == other);
}

// static
EthAddress EthAddress::FromPublicKey(base::span<const uint8_t> public_key) {
  // TODO(apaymyshev): should be a fixed-size span.
  if (public_key.size() != 64) {
    VLOG(1) << __func__ << ": public key size should be 64 bytes";
    return EthAddress();
  }

  return EthAddress(base::span(KeccakHash(public_key)).last(kEthAddressLength));
}

// static
EthAddress EthAddress::FromHex(const std::string& input) {
  if (!IsValidAddress(input)) {
    return EthAddress();
  }

  std::vector<uint8_t> bytes;
  if (!PrefixedHexStringToBytes(input, &bytes)) {
    VLOG(1) << __func__ << ": PrefixedHexStringToBytes failed";
    return EthAddress();
  }

  return EthAddress(std::move(bytes));
}

// static
EthAddress EthAddress::FromBytes(base::span<const uint8_t> bytes) {
  if (bytes.size() != kEthAddressLength) {
    return EthAddress();
  }
  return EthAddress(bytes);
}

// static
EthAddress EthAddress::ZeroAddress() {
  return EthAddress(std::vector<uint8_t>(kEthAddressLength, 0));
}

// static
bool EthAddress::IsValidAddress(const std::string& input) {
  if (!IsValidHexString(input)) {
    VLOG(1) << __func__ << ": input is not a valid hex representation";
    return false;
  }
  if (input.size() - 2 != kEthAddressLength * 2) {
    VLOG(1) << __func__ << ": input should be 20 bytes long";
    return false;
  }
  return true;
}

std::string EthAddress::ToHex() const {
  return ::brave_wallet::ToHex(bytes_);
}

// static
std::optional<std::string> EthAddress::ToEip1191ChecksumAddress(
    const std::string& address,
    const std::string& chain_id) {
  if (address.empty()) {
    return "";
  }

  const auto eth_addr = EthAddress::FromHex(address);
  if (eth_addr.IsEmpty()) {
    return std::nullopt;
  }
  uint256_t chain;
  if (!HexValueToUint256(chain_id, &chain)) {
    return std::nullopt;
  }

  return eth_addr.ToChecksumAddress(chain);
}

std::string EthAddress::ToChecksumAddress(uint256_t eip1191_chaincode) const {
  std::string result = "0x";
  std::string prefix;

  if (eip1191_chaincode == static_cast<uint256_t>(30) ||
      eip1191_chaincode == static_cast<uint256_t>(31)) {
    // TODO(jocelyn): We will need to revise this if there are supported chains
    // with ID larger than uint64_t.
    prefix =
        base::NumberToString(static_cast<uint64_t>(eip1191_chaincode)) + "0x";
  }

  const std::string address_str = HexEncodeLower(bytes_);
  const std::string hash_str = base::HexEncode(
      KeccakHash(base::as_byte_span(base::StrCat({prefix, address_str}))));

  for (size_t i = 0; i < address_str.length(); ++i) {
    if (isdigit(address_str[i])) {
      result.push_back(address_str[i]);
    } else {  // address has already be validated
      int nibble = std::stoi(std::string(1, hash_str[i]), nullptr, 16);
      if (nibble > 7) {
        result.push_back(base::ToUpperASCII(address_str[i]));
      } else {
        result.push_back(address_str[i]);
      }
    }
  }
  return result;
}

bool EthAddress::IsEmpty() const {
  return bytes_.empty();
}

bool EthAddress::IsValid() const {
  return !bytes_.empty();
}

bool EthAddress::IsZeroAddress() const {
  return IsValid() &&
         base::ranges::all_of(bytes_, [](auto b) { return b == 0; });
}

}  // namespace brave_wallet
