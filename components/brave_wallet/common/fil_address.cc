/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/fil_address.h"

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/third_party/argon2/src/src/blake2/blake2.h"
#include "components/base32/base32.h"

namespace brave_wallet {

namespace {

constexpr int kChecksumSize = 4;
constexpr size_t kPublicKeySizeSecp256K = 20;
constexpr size_t kAddressSizeSecp256K = 41;
constexpr size_t kPublicKeySizeBLS = 48;
constexpr size_t kAddressSizeBLS = 86;

absl::optional<std::vector<uint8_t>> BlakeHash(
    const std::vector<uint8_t>& payload,
    size_t length) {
  blake2b_state blakeState;
  if (blake2b_init(&blakeState, length) != 0) {
    VLOG(0) << __func__ << ": blake2b_init failed";
    return absl::nullopt;
  }
  if (blake2b_update(&blakeState, payload.data(), payload.size()) != 0) {
    VLOG(0) << __func__ << ": blake2b_update failed";
    return absl::nullopt;
  }
  std::vector<uint8_t> result(length, 0);
  if (blake2b_final(&blakeState, result.data(), length) != 0) {
    VLOG(0) << __func__ << ": blake2b_final failed";
    return absl::nullopt;
  }
  return result;
}

bool IsValidNetwork(const std::string& network) {
  return network == mojom::kFilecoinTestnet ||
         network == mojom::kFilecoinMainnet;
}

absl::optional<mojom::FilecoinAddressProtocol> ToProtocol(char input) {
  if ((input - '0') == static_cast<int>(mojom::FilecoinAddressProtocol::BLS))
    return mojom::FilecoinAddressProtocol::BLS;
  if ((input - '0') ==
      static_cast<int>(mojom::FilecoinAddressProtocol::SECP256K1))
    return mojom::FilecoinAddressProtocol::SECP256K1;
  return absl::nullopt;
}

}  // namespace

FilAddress::FilAddress(const std::vector<uint8_t>& bytes,
                       mojom::FilecoinAddressProtocol protocol,
                       const std::string& network)
    : protocol_(protocol), network_(network), bytes_(bytes) {
  DCHECK(IsValidNetwork(network));
}
FilAddress::FilAddress() = default;
FilAddress::FilAddress(const FilAddress& other) = default;
FilAddress::~FilAddress() = default;

bool FilAddress::IsEqual(const FilAddress& other) const {
  return bytes_.size() == other.bytes_.size() &&
         std::equal(bytes_.begin(), bytes_.end(), other.bytes_.begin()) &&
         protocol_ == other.protocol_ && network_ == other.network_;
}

bool FilAddress::operator==(const FilAddress& other) const {
  return IsEqual(other);
}

bool FilAddress::operator!=(const FilAddress& other) const {
  return !IsEqual(other);
}

// Decodes Filecoin BLS/SECP256K addresses within rules
// https://spec.filecoin.io/appendix/address/#section-appendix.address.string
// |------------|----------|---------|----------|
// |  network   | protocol | payload | checksum |
// |------------|----------|---------|----------|
// | 'f' or 't' |  1 byte  | n bytes | 4 bytes  |
// static
FilAddress FilAddress::FromAddress(const std::string& address) {
  if (address.size() != kAddressSizeBLS &&
      address.size() != kAddressSizeSecp256K)
    return FilAddress();

  auto protocol = ToProtocol(address[1]);
  if (!protocol)
    return FilAddress();

  std::string network{address[0]};
  if (!IsValidNetwork(network))
    return FilAddress();

  std::string payload{
      base32::Base32Decode(base::ToUpperASCII(address.substr(2)))};
  if (payload.empty())
    return FilAddress();

  std::string key{payload.substr(0, payload.size() - kChecksumSize)};
  std::vector<uint8_t> public_key(key.begin(), key.end());
  return FilAddress::FromPublicKey(public_key, protocol.value(), network);
}

// Creates FilAddress from SECP256K uncompressed public key
// with specified protocol  and network
// https://spec.filecoin.io/appendix/address/#section-appendix.address.string
// static
FilAddress FilAddress::FromUncompressedPublicKey(
    const std::vector<uint8_t>& uncompressed_public_key,
    mojom::FilecoinAddressProtocol protocol,
    const std::string& network) {
  if (protocol == mojom::FilecoinAddressProtocol::BLS)
    return FilAddress();
  if (uncompressed_public_key.empty())
    return FilAddress();
  auto public_key = BlakeHash(uncompressed_public_key, kPublicKeySizeSecp256K);
  if (!public_key || public_key->empty())
    return FilAddress();
  return FromPublicKey(*public_key, protocol, network);
}

// Creates FilAddress from SECP256K or BLS public key
// with specified protocol  and network
// https://spec.filecoin.io/appendix/address/#section-appendix.address.string
// static
FilAddress FilAddress::FromPublicKey(const std::vector<uint8_t>& public_key,
                                     mojom::FilecoinAddressProtocol protocol,
                                     const std::string& network) {
  if (!IsValidNetwork(network))
    return FilAddress();
  if (protocol == mojom::FilecoinAddressProtocol::SECP256K1) {
    if (public_key.size() != kPublicKeySizeSecp256K)
      return FilAddress();
  } else if (protocol == mojom::FilecoinAddressProtocol::BLS) {
    if (public_key.size() != kPublicKeySizeBLS)
      return FilAddress();
  }
  return FilAddress(public_key, protocol, network);
}

// static
bool FilAddress::IsValidAddress(const std::string& address) {
  return !address.empty() &&
         FilAddress::FromAddress(address).ToChecksumAddress() == address;
}

// https://spec.filecoin.io/appendix/address/#section-appendix.address.string
// |------------|----------|---------|----------|
// |  network   | protocol | payload | checksum |
// |------------|----------|---------|----------|
// | 'f' or 't' |  1 byte  | n bytes | 4 bytes  |
std::string FilAddress::ToChecksumAddress() const {
  if (bytes_.empty())
    return std::string();
  std::vector<uint8_t> payload(bytes_);
  std::vector<uint8_t> checksumPayload(bytes_);
  checksumPayload.insert(checksumPayload.begin(), static_cast<int>(protocol_));
  auto checksum = BlakeHash(checksumPayload, kChecksumSize);
  if (!checksum)
    return std::string();
  payload.insert(payload.end(), checksum->begin(), checksum->end());
  std::string input(payload.begin(), payload.end());
  std::string encoded_output = base::ToLowerASCII(
      base32::Base32Encode(input, base32::Base32EncodePolicy::OMIT_PADDING));
  return network_ + std::to_string(static_cast<int>(protocol_)) +
         encoded_output;
}

}  // namespace brave_wallet
