/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/filecoin_keyring.h"

#include <utility>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/bls/buildflags.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/third_party/argon2/src/src/blake2/blake2.h"
#include "components/base32/base32.h"
#if BUILDFLAG(ENABLE_RUST_BLS)
#include "brave/components/bls/rs/src/lib.rs.h"
#endif

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
    const std::string& network) {
#if BUILDFLAG(ENABLE_RUST_BLS)
  if (private_key.empty()) {
    return std::string();
  }

  std::unique_ptr<HDKey> hd_key = HDKey::GenerateFromPrivateKey(private_key);
  if (!hd_key)
    return std::string();

  int protocol = static_cast<int>(mojom::FilecoinAddressProtocol::BLS);
  std::array<uint8_t, 32> payload;
  std::copy_n(private_key.begin(), 32, payload.begin());
  auto result = bls::fil_private_key_public_key(payload);
  std::vector<uint8_t> public_key(result.begin(), result.end());
  if (std::all_of(public_key.begin(), public_key.end(),
                  [](int i) { return i == 0; }))
    return std::string();
  std::string address = network + std::to_string(protocol) +
                        CreateAddressWithProtocol(public_key, protocol);
  if (!AddImportedAddress(address, std::move(hd_key))) {
    return std::string();
  }
  return address;
#else
  return std::string();
#endif
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
                        CreateAddressWithProtocol(payload, protocol);

  if (!AddImportedAddress(address, std::move(hd_key))) {
    return std::string();
  }
  return address;
}

void FilecoinKeyring::ImportFilecoinAccount(
    const std::vector<uint8_t>& input_key,
    const std::string& address) {
  std::unique_ptr<HDKey> hd_key = HDKey::GenerateFromPrivateKey(input_key);
  if (!hd_key)
    return;
  if (!AddImportedAddress(address, std::move(hd_key))) {
    return;
  }
}

std::string FilecoinKeyring::GetAddressInternal(HDKeyBase* hd_key_base) const {
  if (!hd_key_base)
    return std::string();
  HDKey* hd_key = static_cast<HDKey*>(hd_key_base);
  auto uncompressed_public_key = hd_key->GetUncompressedPublicKey();
  auto payload = BlakeHash(uncompressed_public_key, 20);
  int protocol = static_cast<int>(mojom::FilecoinAddressProtocol::SECP256K1);
  // TODO(spylogsster): Get network from settings.
  std::string network = "t";
  return network + std::to_string(protocol) +
         CreateAddressWithProtocol(payload, protocol);
}

std::vector<uint8_t> FilecoinKeyring::SignMessage(
    const std::string& address,
    const std::vector<uint8_t>& message) {
  // NOT IMPLEMENTED
  return std::vector<uint8_t>();
}

std::string FilecoinKeyring::CreateAddressWithProtocol(
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
