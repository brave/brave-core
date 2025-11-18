/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/eth_address.h"

#include <algorithm>
#include <string>
#include <utility>

#include "base/containers/span.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/types/zip.h"
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

// static
EthAddress EthAddress::FromPublicKey(base::span<const uint8_t> public_key) {
  // TODO(apaymyshev): should be a fixed-size span.
  if (public_key.size() != 64) {
    return EthAddress();
  }

  return EthAddress(
      base::as_byte_span(KeccakHash(public_key)).last(kEthAddressLength));
}

// static
EthAddress EthAddress::FromHex(std::string_view input) {
  if (!IsValidAddress(input)) {
    return EthAddress();
  }

  std::vector<uint8_t> bytes;
  if (!PrefixedHexStringToBytes(input, &bytes)) {
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
bool EthAddress::IsValidAddress(std::string_view input) {
  if (!IsValidHexString(input)) {
    return false;
  }
  if (input.size() - 2 != kEthAddressLength * 2) {
    return false;
  }
  return true;
}

std::string EthAddress::ToHex() const {
  return ::brave_wallet::ToHex(bytes_);
}

// static
std::optional<std::string> EthAddress::ToEip1191ChecksumAddress(
    std::string_view address,
    std::string_view chain_id) {
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
  std::string prefix;

  if (eip1191_chaincode == static_cast<uint256_t>(30) ||
      eip1191_chaincode == static_cast<uint256_t>(31)) {
    // TODO(jocelyn): We will need to revise this if there are supported chains
    // with ID larger than uint64_t.
    prefix =
        base::NumberToString(static_cast<uint64_t>(eip1191_chaincode)) + "0x";
  }

  const std::string address_str = base::HexEncodeLower(bytes_);
  const std::string hash_str = base::HexEncode(
      KeccakHash(base::as_byte_span(base::StrCat({prefix, address_str}))));

  std::string result;
  result.reserve(2 + address_str.length());
  result.append("0x");

  for (const auto [address_char, hash_char] :
       base::zip(address_str, hash_str)) {
    // Uppercase address letter if corresponding hash nibble ≥ 8.
    const bool should_uppercase = !base::IsAsciiDigit(address_char) &&
                                  base::HexDigitToInt(hash_char) >= 8;
    result.push_back(should_uppercase ? base::ToUpperASCII(address_char)
                                      : address_char);
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
         std::ranges::all_of(bytes_, [](auto b) { return b == 0; });
}

}  // namespace brave_wallet
