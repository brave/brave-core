/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/fil_address.h"

#include <optional>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/third_party/argon2/src/src/blake2/blake2.h"
#include "components/base32/base32.h"

namespace brave_wallet {

namespace {

constexpr size_t kChecksumSize = 4;
constexpr size_t kHashLengthSecp256K = 20;
constexpr size_t kAddressSizeSecp256K = 41;
constexpr size_t kPublicKeySizeBLS = 48;
constexpr size_t kAddressSizeBLS = 86;
// Only f410 is supported
constexpr size_t kAddressSizeDelegatedF410 = 44;
constexpr size_t kPayloadSizeDelegatedF410 = 20;

std::optional<std::vector<uint8_t>> BlakeHash(
    const std::vector<uint8_t>& payload,
    size_t length) {
  blake2b_state blakeState;
  if (blake2b_init(&blakeState, length) != 0) {
    VLOG(0) << __func__ << ": blake2b_init failed";
    return std::nullopt;
  }
  if (blake2b_update(&blakeState, payload.data(), payload.size()) != 0) {
    VLOG(0) << __func__ << ": blake2b_update failed";
    return std::nullopt;
  }
  std::vector<uint8_t> result(length, 0);
  if (blake2b_final(&blakeState, result.data(), length) != 0) {
    VLOG(0) << __func__ << ": blake2b_final failed";
    return std::nullopt;
  }
  return result;
}

bool IsValidNetwork(const std::string& network) {
  return network == mojom::kFilecoinTestnet ||
         network == mojom::kFilecoinMainnet;
}

std::optional<mojom::FilecoinAddressProtocol> ToProtocol(char input) {
  if ((input - '0') == static_cast<int>(mojom::FilecoinAddressProtocol::BLS)) {
    return mojom::FilecoinAddressProtocol::BLS;
  } else if ((input - '0') ==
             static_cast<int>(mojom::FilecoinAddressProtocol::SECP256K1)) {
    return mojom::FilecoinAddressProtocol::SECP256K1;
  } else if ((input - '0') ==
             static_cast<int>(mojom::FilecoinAddressProtocol::DELEGATED)) {
    return mojom::FilecoinAddressProtocol::DELEGATED;
  }
  return std::nullopt;
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

// static
std::optional<mojom::FilecoinAddressProtocol>
FilAddress::GetProtocolFromAddress(const std::string& address) {
  if (address.size() < 2) {
    return std::nullopt;
  }
  const char protocol_symbol = address[1];
  switch (protocol_symbol) {
    case '1': {
      return mojom::FilecoinAddressProtocol::SECP256K1;
    }
    case '3': {
      return mojom::FilecoinAddressProtocol::BLS;
    }
    default: {
      return std::nullopt;
    }
  }
}

// Decodes Filecoin BLS/SECP256K addresses within rules.
// https://spec.filecoin.io/appendix/address/#section-appendix.address.string
// |------------|----------|---------|----------|
// |  network   | protocol | payload | checksum |
// |------------|----------|---------|----------|
// | 'f' or 't' |  1 byte  | n bytes | 4 bytes  |
// For delegated addresses
// |------------|----------|----------|-----------|---------|----------|
// |  network   | protocol | agent id | delimiter | payload | checksum |
// |------------|----------|----------|-----------|---------|----------|
// | 'f' or 't' |   '4'    |   '10'   |    'f'    | n bytes | 4 bytes  |
// https://github.com/filecoin-project/FIPs/blob/master/FIPS/fip-0048.md
// static
FilAddress FilAddress::FromAddress(const std::string& address) {
  if (address.size() != kAddressSizeBLS &&
      address.size() != kAddressSizeSecp256K &&
      address.size() != kAddressSizeDelegatedF410) {
    return FilAddress();
  }

  auto protocol = ToProtocol(address[1]);
  if (!protocol) {
    return FilAddress();
  }

  std::string network{address[0]};
  if (!IsValidNetwork(network)) {
    return FilAddress();
  }

  std::vector<uint8_t> payload_decoded;
  if (protocol == mojom::FilecoinAddressProtocol::DELEGATED) {
    if (address.substr(2, 3) != "10f") {
      return FilAddress();
    }
    payload_decoded =
        base32::Base32Decode(base::ToUpperASCII(address.substr(5)));
  } else {
    payload_decoded =
        base32::Base32Decode(base::ToUpperASCII(address.substr(2)));
  }

  if (payload_decoded.size() < kChecksumSize) {
    return FilAddress();
  }

  payload_decoded.erase(std::prev(payload_decoded.end(), kChecksumSize),
                        payload_decoded.end());
  return FilAddress::FromPayload(payload_decoded, protocol.value(), network);
}

// static
FilAddress FilAddress::FromBytes(const std::string& chain_id,
                                 const std::vector<uint8_t>& bytes) {
  if (!IsValidNetwork(chain_id)) {
    return FilAddress();
  }
  if (bytes.empty()) {
    return FilAddress();
  }
  mojom::FilecoinAddressProtocol protocol =
      static_cast<mojom::FilecoinAddressProtocol>(bytes.at(0));
  std::vector<uint8_t> payload(bytes.begin() + 1, bytes.end());

  return FilAddress::FromPayload(payload, protocol, chain_id);
}

// Creates FilAddress from SECP256K uncompressed public key
// with specified protocol and network.
// https://spec.filecoin.io/appendix/address/#section-appendix.address.string
// static
FilAddress FilAddress::FromUncompressedPublicKey(
    const std::vector<uint8_t>& uncompressed_public_key,
    mojom::FilecoinAddressProtocol protocol,
    const std::string& network) {
  if (protocol != mojom::FilecoinAddressProtocol::SECP256K1) {
    return FilAddress();
  }
  if (uncompressed_public_key.empty()) {
    return FilAddress();
  }
  auto payload = BlakeHash(uncompressed_public_key, kHashLengthSecp256K);
  if (!payload || payload->empty()) {
    return FilAddress();
  }
  return FromPayload(*payload, protocol, network);
}

// static
FilAddress FilAddress::FromFEVMAddress(bool is_mainnet,
                                       const std::string& fevm_address) {
  if (!EthAddress::IsValidAddress(fevm_address)) {
    return FilAddress();
  }
  std::vector<uint8_t> payload;
  base::HexStringToBytes(fevm_address.substr(2), &payload);
  std::vector<uint8_t> prefix = {4, 10};

  blake2b_state blakeState;
  if (blake2b_init(&blakeState, 4) != 0) {
    return FilAddress();
  }

  if (blake2b_update(&blakeState, prefix.data(), prefix.size()) != 0) {
    return FilAddress();
  }

  if (blake2b_update(&blakeState, payload.data(), payload.size()) != 0) {
    return FilAddress();
  }

  std::vector<uint8_t> checksum(4, 0);
  if (blake2b_final(&blakeState, checksum.data(), checksum.size()) != 0) {
    return FilAddress();
  }

  payload.insert(payload.end(), checksum.begin(), checksum.end());

  std::string encoded =
      base32::Base32Encode(payload, base32::Base32EncodePolicy::OMIT_PADDING);
  return FilAddress::FromAddress((is_mainnet ? "f410f" : "t410f") + encoded);
}

// Creates FilAddress from SECP256K or BLS payload
// with specified protocol and network.
// https://spec.filecoin.io/appendix/address/#section-appendix.address.string
// static
FilAddress FilAddress::FromPayload(const std::vector<uint8_t>& payload,
                                   mojom::FilecoinAddressProtocol protocol,
                                   const std::string& network) {
  if (!IsValidNetwork(network)) {
    return FilAddress();
  }
  if (protocol == mojom::FilecoinAddressProtocol::SECP256K1) {
    if (payload.size() != kHashLengthSecp256K) {
      return FilAddress();
    }
  } else if (protocol == mojom::FilecoinAddressProtocol::BLS) {
    if (payload.size() != kPublicKeySizeBLS) {
      return FilAddress();
    }
  } else if (protocol == mojom::FilecoinAddressProtocol::DELEGATED) {
    if (payload.size() != kPayloadSizeDelegatedF410) {
      return FilAddress();
    }
  } else {
    NOTREACHED() << protocol;
  }
  return FilAddress(payload, protocol, network);
}

// static
bool FilAddress::IsValidAddress(const std::string& address) {
  return !address.empty() &&
         FilAddress::FromAddress(address).EncodeAsString() == address;
}

// https://spec.filecoin.io/appendix/address/#section-appendix.address.string
// |------------|----------|---------|----------|
// |  network   | protocol | payload | checksum |
// |------------|----------|---------|----------|
// | 'f' or 't' |  1 byte  | n bytes | 4 bytes  |
// For delegated addresses
// |------------|----------|----------|-----------|---------|----------|
// |  network   | protocol | agent id | delimiter | payload | checksum |
// |------------|----------|----------|-----------|---------|----------|
// | 'f' or 't' |   '4'    |   '10'   |    'f'    | n bytes | 4 bytes  |
// Protocol value 1: addresses represent secp256k1 public encryption keys.
// The payload field contains the Blake2b 160 hash of the uncompressed public
// key (65 bytes).
// Protocol value 3: addresses represent BLS public encryption keys.
// The payload field contains 48 byte BLS PubKey public key. All payloads
// except the payload of the ID protocol are base32 encoded using the lowercase
// alphabet when seralized to their human readable format.
// Protocol value 4: addresses represent a combination of agent id and agent
// namespace addresses.
// https://github.com/filecoin-project/FIPs/blob/master/FIPS/fip-0048.md
// Filecoin checksums are calculated over the address protocol and
// payload using blake2b-4. Checksums are base32 encoded and
// only added to an address when encoding to a string.
// Addresses following the ID Protocol do not have a checksum.
std::string FilAddress::EncodeAsString() const {
  if (bytes_.empty()) {
    return std::string();
  }
  std::vector<uint8_t> payload_hash(bytes_);
  std::vector<uint8_t> checksum(bytes_);
  if (protocol_ == mojom::FilecoinAddressProtocol::DELEGATED) {
    checksum.insert(checksum.begin(), 0x0A /* Agent id is 10*/);
  }
  checksum.insert(checksum.begin(), static_cast<int>(protocol_));
  auto checksum_hash = BlakeHash(checksum, kChecksumSize);
  if (!checksum_hash) {
    return std::string();
  }
  payload_hash.insert(payload_hash.end(), checksum_hash->begin(),
                      checksum_hash->end());
  // Encoding as lower case base32 without padding according to
  // https://spec.filecoin.io/appendix/address/#section-appendix.address.payload
  // and https://github.com/multiformats/multibase/blob/master/multibase.csv
  std::string encoded_output = base::ToLowerASCII(base32::Base32Encode(
      payload_hash, base32::Base32EncodePolicy::OMIT_PADDING));
  if (protocol_ == mojom::FilecoinAddressProtocol::DELEGATED) {
    auto r = network_ + std::to_string(static_cast<int>(protocol_)) +
             // Agent id + delimiter
             "10f" + encoded_output;
    return r;
  } else {
    return network_ + std::to_string(static_cast<int>(protocol_)) +
           encoded_output;
  }
}

std::vector<uint8_t> FilAddress::GetBytes() const {
  std::vector<uint8_t> result;
  result.push_back(static_cast<uint8_t>(protocol_));
  result.insert(result.end(), bytes_.begin(), bytes_.end());
  return result;
}

bool FilAddress::IsMainNet() const {
  return network_ == mojom::kFilecoinMainnet;
}

}  // namespace brave_wallet
