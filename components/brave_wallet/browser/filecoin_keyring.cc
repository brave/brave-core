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
#include "base/json/json_reader.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/fil_transaction.h"
#include "brave/components/brave_wallet/browser/internal/hd_key.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "brave/components/filecoin/rs/src/lib.rs.h"

namespace brave_wallet {

namespace {
bool GetBLSPublicKey(const std::vector<uint8_t>& private_key,
                     std::vector<uint8_t>* public_key_out) {
  if (private_key.size() != 32 || !public_key_out) {
    return false;
  }

  auto result = filecoin::bls_private_key_to_public_key(
      rust::Slice<const uint8_t>{private_key.data(), private_key.size()});
  std::vector<uint8_t> public_key(result.begin(), result.end());
  if (std::all_of(public_key.begin(), public_key.end(),
                  [](int i) { return i == 0; })) {
    return false;
  }
  *public_key_out = public_key;
  return true;
}

std::string GetExportEncodedJSON(base::span<const uint8_t> private_key_bytes,
                                 const std::string& address) {
  std::optional<mojom::FilecoinAddressProtocol> protocol =
      FilAddress::GetProtocolFromAddress(address);
  if (!protocol) {
    return "";
  }
  std::string json = base::StringPrintf(
      "{\"Type\":\"%s\",\"PrivateKey\":\"%s\"}",
      protocol.value() == mojom::FilecoinAddressProtocol::BLS ? "bls"
                                                              : "secp256k1",
      base::Base64Encode(private_key_bytes).c_str());
  return base::ToLowerASCII(base::HexEncode(json));
}

}  // namespace

FilecoinKeyring::~FilecoinKeyring() = default;

FilecoinKeyring::FilecoinKeyring(base::span<const uint8_t> seed,
                                 const std::string& chain_id)
    : Secp256k1HDKeyring(
          seed,
          GetRootPath(chain_id == mojom::kFilecoinMainnet
                          ? mojom::KeyringId::kFilecoin
                          : mojom::KeyringId::kFilecoinTestnet)) {
  network_ = chain_id;
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
  std::optional<base::Value> records_v = base::JSONReader::Read(
      key_payload, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v || !records_v->is_dict()) {
    VLOG(1) << "Invalid payload, could not parse JSON, JSON is: "
            << key_payload;
    return false;
  }

  const auto& dict = records_v->GetDict();
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

std::string FilecoinKeyring::EncodePrivateKeyForExport(
    const std::string& address) {
  if (auto it = imported_bls_accounts_.find(address);
      it != imported_bls_accounts_.end()) {
    return GetExportEncodedJSON(*it->second, address);
  }

  HDKey* key = GetHDKeyFromAddress(address);
  if (!key) {
    return "";
  }
  return GetExportEncodedJSON(key->GetPrivateKeyBytes(), address);
}

std::vector<std::string> FilecoinKeyring::GetImportedAccountsForTesting()
    const {
  auto result = Secp256k1HDKeyring::GetImportedAccountsForTesting();

  for (auto& blc_acc : imported_bls_accounts_) {
    result.push_back(blc_acc.first);
  }
  return result;
}

std::string FilecoinKeyring::ImportFilecoinAccount(
    const std::vector<uint8_t>& private_key,
    mojom::FilecoinAddressProtocol protocol) {
  if (private_key.empty()) {
    return std::string();
  }

  if (protocol == mojom::FilecoinAddressProtocol::BLS) {
    return ImportBlsAccount(private_key);
  } else if (protocol == mojom::FilecoinAddressProtocol::SECP256K1) {
    return ImportAccount(private_key);
  }
  NOTREACHED_IN_MIGRATION() << protocol;
  return std::string();
}

bool FilecoinKeyring::RemoveImportedAccount(const std::string& address) {
  if (Secp256k1HDKeyring::RemoveImportedAccount(address)) {
    return true;
  }

  return imported_bls_accounts_.erase(address) != 0;
}

std::string FilecoinKeyring::ImportBlsAccount(
    const std::vector<uint8_t>& private_key) {
  std::vector<uint8_t> public_key;
  if (!GetBLSPublicKey(private_key, &public_key)) {
    return std::string();
  }
  FilAddress fil_address = FilAddress::FromPayload(
      public_key, mojom::FilecoinAddressProtocol::BLS, network_);
  if (fil_address.IsEmpty()) {
    return std::string();
  }
  std::string address = fil_address.EncodeAsString();

  if (base::Contains(imported_bls_accounts_, address)) {
    return std::string();
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
    const FilTransaction* tx) {
  if (!tx) {
    return std::nullopt;
  }
  auto fil_address = FilAddress::FromAddress(address);
  if (fil_address.IsEmpty()) {
    return std::nullopt;
  }

  if (auto it = imported_bls_accounts_.find(address);
      it != imported_bls_accounts_.end()) {
    return tx->GetSignedTransaction(fil_address, *it->second);
  }

  HDKey* hd_key = GetHDKeyFromAddress(address);
  if (!hd_key) {
    return std::nullopt;
  }
  return tx->GetSignedTransaction(fil_address, hd_key->GetPrivateKeyBytes());
}

std::unique_ptr<HDKey> FilecoinKeyring::DeriveAccount(uint32_t index) const {
  // Mainnet m/44'/461'/0'/0/{index}
  // Testnet m/44'/1'/0'/0/{index}
  return root_->DeriveNormalChild(index);
}

}  // namespace brave_wallet
