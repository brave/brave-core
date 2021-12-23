/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/filecoin_keyring.h"

#include <utility>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/third_party/argon2/src/src/blake2/blake2.h"
#include "components/base32/base32.h"

namespace {

std::vector<uint8_t> BlakeHash(const std::vector<uint8_t>& payload,
                               size_t length) {
  blake2b_state blakeState;
  if (blake2b_init(&blakeState, length) != 0) {
    VLOG(0) << __func__ << ": blake2b_init failed";
    return std::vector<uint8_t>();
  }
  if (blake2b_update(&blakeState, payload.data(), payload.size()) != 0) {
    VLOG(0) << __func__ << ": blake2b_update failed";
    return std::vector<uint8_t>();
  }
  std::vector<uint8_t> result;
  result.resize(length);
  if (blake2b_final(&blakeState, result.data(), length) != 0) {
    VLOG(0) << __func__ << ": blake2b_final failed";
    return result;
  }
  return result;
}

}  // namespace

namespace brave_wallet {

FilecoinKeyring::FilecoinKeyring() = default;
FilecoinKeyring::~FilecoinKeyring() = default;

FilecoinKeyring::Type FilecoinKeyring::type() const {
  return kFilecoin;
}

std::string FilecoinKeyring::ImportFilecoinBLSAccount(
    const std::vector<uint8_t>& private_key,
    const std::vector<uint8_t>& public_key,
    const std::string& network) {
  if (private_key.empty() || public_key.empty()) {
    return std::string();
  }

  std::unique_ptr<HDKey> hd_key = HDKey::GenerateFromPrivateKey(private_key);
  if (!hd_key)
    return std::string();
  int protocol = static_cast<int>(mojom::FilecoinAddressProtocol::BLS);
  std::string address = network + std::to_string(protocol) +
                        GetAddressInternal(public_key, protocol);
  if (!AddImportedAddress(address, std::move(hd_key))) {
    return std::string();
  }
  return address;
}

std::string FilecoinKeyring::ImportFilecoinSECP256K1Account(
    const std::vector<uint8_t>& input_key,
    const std::string& network) {
  if (input_key.empty()) {
    return std::string();
  }
  std::unique_ptr<HDKey> hd_key = HDKey::GenerateFromPrivateKey(input_key);
  if (!hd_key)
    return std::string();
  auto uncompressed_public_key = hd_key->GetUncompressedPublicKey();
  auto payload = BlakeHash(uncompressed_public_key, 20);
  int protocol = static_cast<int>(mojom::FilecoinAddressProtocol::SECP256K1);
  std::string address = network + std::to_string(protocol) +
                        GetAddressInternal(payload, protocol);

  if (!AddImportedAddress(address, std::move(hd_key))) {
    return std::string();
  }
  return address;
}

std::string FilecoinKeyring::GetAddressInternal(
    const std::vector<uint8_t>& payload,
    int protocol_index) const {
  std::vector<uint8_t> checksumPayload(payload);
  checksumPayload.insert(checksumPayload.begin(), protocol_index);
  auto checksum = BlakeHash(checksumPayload, 4);
  std::vector<uint8_t> final(payload);
  final.insert(final.end(), checksum.begin(), checksum.end());
  std::string input(final.begin(), final.end());
  std::string encoded_output = base::ToLowerASCII(
      base32::Base32Encode(input, base32::Base32EncodePolicy::OMIT_PADDING));
  return encoded_output;
}

}  // namespace brave_wallet
