/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_keyring.h"

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <utility>

#include "base/containers/contains.h"
#include "base/containers/span.h"
#include "base/containers/span_rust.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/lib.rs.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "crypto/hash.h"

namespace brave_wallet {

namespace {

constexpr size_t kMaxSeeds = 16;
constexpr size_t kMaxSeedLen = 32;

}  // namespace

SolanaKeyring::SolanaKeyring(base::span<const uint8_t> seed)
    : root_(ConstructRootHDKey(seed)) {}
SolanaKeyring::~SolanaKeyring() = default;

// static
std::unique_ptr<HDKeyEd25519> SolanaKeyring::ConstructRootHDKey(
    base::span<const uint8_t> seed) {
  if (seed.empty()) {
    return nullptr;
  }
  auto result = HDKeyEd25519::GenerateFromSeed(seed);
  if (!result) {
    return nullptr;
  }

  // m/44'/501'
  for (auto index : std::to_array({44, 501})) {
    result = result->DeriveHardenedChild(index);
    if (!result) {
      return nullptr;
    }
  }

  return result;
}

std::optional<std::string> SolanaKeyring::AddNewHDAccount(uint32_t index) {
  if (!root_) {
    return std::nullopt;
  }

  if (accounts_.size() != index) {
    return std::nullopt;
  }

  auto new_account = DeriveAccount(index);
  if (!new_account) {
    return std::nullopt;
  }

  auto address = GetAddressInternal(*new_account);
  accounts_.push_back(std::move(new_account));

  return address;
}

bool SolanaKeyring::RemoveHDAccount(uint32_t index) {
  if (accounts_.size() - 1 != index) {
    return false;
  }
  accounts_.pop_back();
  return true;
}

std::optional<std::string> SolanaKeyring::ImportAccount(
    base::span<const uint8_t> keypair) {
  // extract private key from keypair
  std::vector<uint8_t> private_key = std::vector<uint8_t>(
      keypair.begin(), keypair.begin() + kSolanaPrikeySize);
  std::unique_ptr<HDKeyEd25519> hd_key =
      HDKeyEd25519::GenerateFromPrivateKey(private_key);
  if (!hd_key) {
    return std::nullopt;
  }

  std::string address = GetAddressInternal(*hd_key);

  if (base::Contains(imported_accounts_, address)) {
    return std::nullopt;
  }

  if (std::ranges::any_of(accounts_, [&](auto& acc) {
        return GetAddressInternal(*acc) == address;
      })) {
    return std::nullopt;
  }

  imported_accounts_[address] = std::move(hd_key);
  return address;
}

bool SolanaKeyring::RemoveImportedAccount(const std::string& address) {
  return imported_accounts_.erase(address) != 0;
}

std::optional<std::string> SolanaKeyring::EncodePrivateKeyForExport(
    const std::string& address) {
  HDKeyEd25519* hd_key = GetHDKeyFromAddress(address);
  if (!hd_key) {
    return std::nullopt;
  }

  return hd_key->GetBase58EncodedKeypair();
}

std::vector<uint8_t> SolanaKeyring::SignMessage(
    const std::string& address,
    base::span<const uint8_t> message) {
  HDKeyEd25519* hd_key =
      static_cast<HDKeyEd25519*>(GetHDKeyFromAddress(address));
  if (!hd_key) {
    return std::vector<uint8_t>();
  }

  return hd_key->Sign(message);
}

std::vector<std::string> SolanaKeyring::GetHDAccountsForTesting() const {
  std::vector<std::string> addresses;
  for (auto& acc : accounts_) {
    addresses.push_back(GetAddressInternal(*acc));
  }
  return addresses;
}

std::vector<std::string> SolanaKeyring::GetImportedAccountsForTesting() const {
  std::vector<std::string> addresses;
  for (auto& acc : imported_accounts_) {
    addresses.push_back(GetAddressInternal(*acc.second));
  }
  return addresses;
}

std::optional<std::string> SolanaKeyring::GetDiscoveryAddress(
    size_t index) const {
  if (auto key = DeriveAccount(index)) {
    return GetAddressInternal(*key);
  }
  return std::nullopt;
}

std::string SolanaKeyring::GetAddressInternal(
    const HDKeyEd25519& hd_key) const {
  return hd_key.GetBase58EncodedPublicKey();
}

// Create a valid program derived address without searching for a bump seed.
// https://docs.rs/solana-program/latest/solana_program/pubkey/struct.Pubkey.html#method.create_program_address
// static
std::optional<std::string> SolanaKeyring::CreateProgramDerivedAddress(
    const std::vector<std::vector<uint8_t>>& seeds,
    const std::string& program_id) {
  const std::string pda_marker = "ProgramDerivedAddress";

  std::vector<uint8_t> program_id_bytes;
  if (!Base58Decode(program_id, &program_id_bytes, kSolanaPubkeySize)) {
    return std::nullopt;
  }

  if (seeds.size() > kMaxSeeds) {
    return std::nullopt;
  }

  for (const auto& seed : seeds) {
    if (seed.size() > kMaxSeedLen) {
      return std::nullopt;
    }
  }

  std::vector<uint8_t> buffer;
  for (const auto& seed : seeds) {
    buffer.insert(buffer.end(), seed.begin(), seed.end());
  }

  buffer.insert(buffer.end(), program_id_bytes.begin(), program_id_bytes.end());
  buffer.insert(buffer.end(), pda_marker.begin(), pda_marker.end());

  auto hash_array = crypto::hash::Sha256(buffer);

  // Invalid because program derived addresses have to be off-curve.
  if (bytes_are_curve25519_point(base::SpanToRustSlice(hash_array))) {
    return std::nullopt;
  }

  return Base58Encode(hash_array);
}

// Find a valid program derived address and its corresponding bump seed.
// https://docs.rs/solana-program/latest/solana_program/pubkey/struct.Pubkey.html#method.find_program_address
// static
std::optional<std::string> SolanaKeyring::FindProgramDerivedAddress(
    const std::vector<std::vector<uint8_t>>& seeds,
    const std::string& program_id,
    uint8_t* ret_bump_seed) {
  std::vector<std::vector<uint8_t>> seeds_with_bump(seeds);
  std::vector<uint8_t> bump_seed = {UINT8_MAX};
  while (bump_seed[0] != 0) {
    seeds_with_bump.push_back(bump_seed);

    auto address = CreateProgramDerivedAddress(seeds_with_bump, program_id);
    if (address) {
      if (ret_bump_seed) {
        *ret_bump_seed = bump_seed[0];
      }

      return address;
    }

    seeds_with_bump.pop_back();
    --bump_seed[0];
  }

  return std::nullopt;
}

// Derives the associated token account address for the given wallet address and
// token mint.
// https://docs.rs/spl-associated-token-account/1.0.3/spl_associated_token_account/fn.get_associated_token_address.html
// static
std::optional<std::string> SolanaKeyring::GetAssociatedTokenAccount(
    const std::string& spl_token_mint_address,
    const std::string& wallet_address,
    mojom::SPLTokenProgram token_program) {
  std::string token_program_id = SPLTokenProgramToProgramID(token_program);
  if (token_program_id.empty()) {
    return std::nullopt;
  }

  std::vector<std::vector<uint8_t>> seeds;
  std::vector<uint8_t> wallet_address_bytes;
  std::vector<uint8_t> token_program_id_bytes;
  std::vector<uint8_t> spl_token_mint_address_bytes;

  if (!Base58Decode(wallet_address, &wallet_address_bytes, kSolanaPubkeySize) ||
      !Base58Decode(token_program_id, &token_program_id_bytes,
                    kSolanaPubkeySize) ||
      !Base58Decode(spl_token_mint_address, &spl_token_mint_address_bytes,
                    kSolanaPubkeySize)) {
    return std::nullopt;
  }

  seeds.push_back(std::move(wallet_address_bytes));
  seeds.push_back(std::move(token_program_id_bytes));
  seeds.push_back(std::move(spl_token_mint_address_bytes));

  return FindProgramDerivedAddress(seeds,
                                   mojom::kSolanaAssociatedTokenProgramId);
}

// static
// Derive metadata account using metadata seed constant, token metadata program
// id, and the mint address as the seeds.
// https://docs.metaplex.com/programs/token-metadata/accounts#metadata
std::optional<std::string> SolanaKeyring::GetAssociatedMetadataAccount(
    const std::string& token_mint_address) {
  std::vector<std::vector<uint8_t>> seeds;
  const std::string metadata_seed_constant = "metadata";
  std::vector<uint8_t> metaplex_seed_constant_bytes(
      metadata_seed_constant.begin(), metadata_seed_constant.end());
  std::vector<uint8_t> metadata_program_id_bytes;
  std::vector<uint8_t> token_mint_address_bytes;

  if (!Base58Decode(mojom::kSolanaMetadataProgramId, &metadata_program_id_bytes,
                    kSolanaPubkeySize) ||
      !Base58Decode(token_mint_address, &token_mint_address_bytes,
                    kSolanaPubkeySize)) {
    return std::nullopt;
  }

  seeds.push_back(std::move(metaplex_seed_constant_bytes));
  seeds.push_back(std::move(metadata_program_id_bytes));
  seeds.push_back(std::move(token_mint_address_bytes));

  return FindProgramDerivedAddress(seeds, mojom::kSolanaMetadataProgramId);
}

std::unique_ptr<HDKeyEd25519> SolanaKeyring::DeriveAccount(
    uint32_t index) const {
  // m/44'/501'/{index}'/0'
  return root_->DeriveHardenedChild(index)->DeriveHardenedChild(0);
}

HDKeyEd25519* SolanaKeyring::GetHDKeyFromAddress(const std::string& address) {
  const auto imported_accounts_iter = imported_accounts_.find(address);
  if (imported_accounts_iter != imported_accounts_.end()) {
    return imported_accounts_iter->second.get();
  }
  for (auto& acc : accounts_) {
    if (GetAddressInternal(*acc) == address) {
      return acc.get();
    }
  }
  return nullptr;
}

}  // namespace brave_wallet
