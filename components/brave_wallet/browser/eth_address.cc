/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_address.h"

#include "base/check_op.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"

namespace brave_wallet {

namespace {
#define ADDRESS_LEN 20
}  // namespace

EthAddress::EthAddress(const std::vector<uint8_t>& bytes) : bytes_(bytes) {}
EthAddress::EthAddress() = default;
EthAddress::EthAddress(const EthAddress& other) = default;
EthAddress::~EthAddress() {}

bool EthAddress::operator==(const EthAddress& other) const {
  return std::equal(bytes_.begin(), bytes_.end(), other.bytes_.begin());
}

bool EthAddress::operator!=(const EthAddress& other) const {
  return !std::equal(bytes_.begin(), bytes_.end(), other.bytes_.begin());
}

// static
EthAddress EthAddress::FromPublicKey(const std::vector<uint8_t>& public_key) {
  if (public_key.size() != 64) {
    LOG(ERROR) << __func__ << ": public key size should be 64 bytes";
    return EthAddress();
  }

  std::vector<uint8_t> hash = KeccakHash(public_key);
  std::vector<uint8_t> result(hash.end() - ADDRESS_LEN, hash.end());

  DCHECK(result.size() == ADDRESS_LEN);

  return EthAddress(result);
}

// static
EthAddress EthAddress::FromHex(const std::string& input) {
  if (!IsValidHexString(input)) {
    LOG(ERROR) << __func__ << ": input is not a valid hex representation";
    return EthAddress();
  }
  std::vector<uint8_t> bytes;
  if (!base::HexStringToBytes(input.data() + 2, &bytes)) {
    LOG(ERROR) << __func__ << ": base::HexStringToBytes failed";
    return EthAddress();
  }
  if (bytes.size() != ADDRESS_LEN) {
    LOG(ERROR) << __func__ << ": input should be 20 bytes long";
    return EthAddress();
  }

  return EthAddress(bytes);
}

std::string EthAddress::ToHex() const {
  const std::string input(bytes_.begin(), bytes_.end());
  return ::brave_wallet::ToHex(input);
}

std::string EthAddress::ToChecksumAddress(uint256_t eip1191_chaincode) const {
  std::string result = "0x";
  std::string input;

  if (eip1191_chaincode == static_cast<uint256_t>(30) ||
      eip1191_chaincode == static_cast<uint256_t>(31)) {
    // TODO(jocelyn): We will need to revise this if there are supported chains
    // with ID larger than uint64_t.
    input +=
        base::NumberToString(static_cast<uint64_t>(eip1191_chaincode)) + "0x";
  }

  input += std::string(ToHex().data() + 2);

  const std::string hash_str(KeccakHash(input).data() + 2);
  const std::string address_str =
      base::ToLowerASCII(base::HexEncode(bytes_.data(), bytes_.size()));

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

}  // namespace brave_wallet
