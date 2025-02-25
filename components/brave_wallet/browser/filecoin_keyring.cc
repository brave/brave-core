/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/filecoin_keyring.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/containers/contains.h"
#include "base/containers/span_rust.h"
#include "base/containers/to_vector.h"
#include "base/json/json_reader.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/fil_transaction.h"
#include "brave/components/brave_wallet/browser/internal/hd_key.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_common.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "brave/components/filecoin/rs/src/lib.rs.h"

namespace brave_wallet {

namespace {
std::optional<std::vector<uint8_t>> GetBLSPublicKey(
    base::span<const uint8_t> private_key) {
  if (private_key.size() != 32) {
    return std::nullopt;
  }

  auto public_key = base::ToVector(filecoin::bls_private_key_to_public_key(
      base::SpanToRustSlice(private_key)));
  if (std::ranges::all_of(public_key, [](int i) { return i == 0; })) {
    return std::nullopt;
  }
  return public_key;
}

std::optional<std::string> GetExportEncodedJSON(
    base::span<const uint8_t> private_key_bytes,
    const std::string& address) {
  std::optional<mojom::FilecoinAddressProtocol> protocol =
      FilAddress::GetProtocolFromAddress(address);
  if (!protocol) {
    return std::nullopt;
  }
  std::string json = base::StringPrintf(
      "{\"Type\":\"%s\",\"PrivateKey\":\"%s\"}",
      protocol.value() == mojom::FilecoinAddressProtocol::BLS ? "bls"
                                                              : "secp256k1",
      base::Base64Encode(private_key_bytes).c_str());
  return base::ToLowerASCII(base::HexEncode(json));
}

std::unique_ptr<HDKey> ConstructAccountsRootKey(base::span<const uint8_t> seed,
                                                bool testnet) {
  auto result = HDKey::GenerateFromSeed(seed);
  if (!result) {
    return nullptr;
  }

  if (testnet) {
    // Testnet: m/44'/1'/0'/0
    return result->DeriveChildFromPath({DerivationIndex::Hardened(44),  //
                                        DerivationIndex::Hardened(1),
                                        DerivationIndex::Hardened(0),
                                        DerivationIndex::Normal(0)});
  } else {
    // Mainnet: m/44'/461'/0'/0
    return result->DeriveChildFromPath({DerivationIndex::Hardened(44),  //
                                        DerivationIndex::Hardened(461),
                                        DerivationIndex::Hardened(0),
                                        DerivationIndex::Normal(0)});
  }
}

}  // namespace

FilecoinKeyring::~FilecoinKeyring() = default;

FilecoinKeyring::FilecoinKeyring(base::span<const uint8_t> seed,
                                 mojom::KeyringId keyring_id)
    : keyring_id_(keyring_id) {
  DCHECK(IsFilecoinKeyring(keyring_id));
  accounts_root_ = ConstructAccountsRootKey(
      seed, keyring_id == mojom::KeyringId::kFilecoinTestnet);
  network_ = GetFilecoinChainId(keyring_id);
  DCHECK(network_ == mojom::kFilecoinMainnet ||
         network_ == mojom::kFilecoinTestnet);
}

// static
bool FilecoinKeyring::DecodeImportPayload(
    const std::string& payload_hex,
    std::vector<uint8_t>* private_key_out,
    mojom::FilecoinAddressProtocol* protocol_out) {
  if (!private_key_out || !protocol_out || payload_hex.empty()) {
    return false;
  }
  std::string key_payload;
  if (!base::HexStringToString(payload_hex, &key_payload)) {
    return false;
  }
  std::optional<base::Value::Dict> records_v = base::JSONReader::ReadDict(
      key_payload, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v) {
    VLOG(1) << "Invalid payload, could not parse JSON, JSON is: "
            << key_payload;
    return false;
  }

  const auto& dict = *records_v;
  const std::string* type = dict.FindString("Type");
  if (!type || (*type != "secp256k1" && *type != "bls")) {
    return false;
  }

  *protocol_out = *type == "secp256k1"
                      ? mojom::FilecoinAddressProtocol::SECP256K1
                      : mojom::FilecoinAddressProtocol::BLS;

  const std::string* private_key_encoded = dict.FindString("PrivateKey");
  if (!private_key_encoded || private_key_encoded->empty()) {
    return false;
  }
  std::string private_key_decoded;
  if (!base::Base64Decode(*private_key_encoded, &private_key_decoded)) {
    return false;
  }

  *private_key_out = std::vector<uint8_t>(private_key_decoded.begin(),
                                          private_key_decoded.end());
  return true;
}

std::optional<std::string> FilecoinKeyring::GetDiscoveryAddress(
    size_t index) const {
  if (auto key = DeriveAccount(index)) {
    return GetAddressInternal(*key);
  }
  return std::nullopt;
}

std::optional<std::string> FilecoinKeyring::EncodePrivateKeyForExport(
    const std::string& address) {
  if (auto it = imported_bls_accounts_.find(address);
      it != imported_bls_accounts_.end()) {
    return GetExportEncodedJSON(*it->second, address);
  }

  HDKey* key = GetHDKeyFromAddress(address);
  if (!key) {
    return std::nullopt;
  }
  return GetExportEncodedJSON(key->GetPrivateKeyBytes(), address);
}

std::vector<std::string> FilecoinKeyring::GetImportedAccountsForTesting()
    const {
  std::vector<std::string> addresses;
  for (auto& acc : imported_accounts_) {
    addresses.push_back(GetAddressInternal(*acc.second));
  }

  for (auto& blc_acc : imported_bls_accounts_) {
    addresses.push_back(blc_acc.first);
  }
  return addresses;
}

std::optional<std::string> FilecoinKeyring::ImportFilecoinAccount(
    base::span<const uint8_t> private_key,
    mojom::FilecoinAddressProtocol protocol) {
  if (private_key.empty()) {
    return std::nullopt;
  }

  if (protocol == mojom::FilecoinAddressProtocol::BLS) {
    return ImportBlsAccount(private_key);
  } else if (protocol == mojom::FilecoinAddressProtocol::SECP256K1) {
    return ImportAccount(private_key);
  }
  NOTREACHED();
}

bool FilecoinKeyring::RemoveImportedAccount(const std::string& address) {
  if (Secp256k1HDKeyring::RemoveImportedAccount(address)) {
    return true;
  }

  return imported_bls_accounts_.erase(address) != 0;
}

std::optional<std::string> FilecoinKeyring::ImportBlsAccount(
    base::span<const uint8_t> private_key) {
  auto public_key = GetBLSPublicKey(private_key);
  if (!public_key) {
    return std::nullopt;
  }

  FilAddress fil_address = FilAddress::FromPayload(
      *public_key, mojom::FilecoinAddressProtocol::BLS, network_);
  if (fil_address.IsEmpty()) {
    return std::nullopt;
  }
  std::string address = fil_address.EncodeAsString();

  if (base::Contains(imported_bls_accounts_, address)) {
    return std::nullopt;
  }

  imported_bls_accounts_[address] =
      std::make_unique<SecureVector>(private_key.begin(), private_key.end());
  return address;
}

std::string FilecoinKeyring::GetAddressInternal(const HDKey& hd_key) const {
  return FilAddress::FromUncompressedPublicKey(
             hd_key.GetUncompressedPublicKey(),
             mojom::FilecoinAddressProtocol::SECP256K1, network_)
      .EncodeAsString();
}

std::optional<std::string> FilecoinKeyring::SignTransaction(
    const std::string& address,
    const FilTransaction& tx) {
  auto fil_address = FilAddress::FromAddress(address);
  if (fil_address.IsEmpty()) {
    return std::nullopt;
  }

  if (auto it = imported_bls_accounts_.find(address);
      it != imported_bls_accounts_.end()) {
    return tx.GetSignedTransaction(fil_address, *it->second);
  }

  HDKey* hd_key = GetHDKeyFromAddress(address);
  if (!hd_key) {
    return std::nullopt;
  }
  return tx.GetSignedTransaction(fil_address, hd_key->GetPrivateKeyBytes());
}

std::unique_ptr<HDKey> FilecoinKeyring::DeriveAccount(uint32_t index) const {
  // Mainnet m/44'/461'/0'/0/{index}
  // Testnet m/44'/1'/0'/0/{index}
  return accounts_root_->DeriveChild(DerivationIndex::Normal(index));
}

}  // namespace brave_wallet
