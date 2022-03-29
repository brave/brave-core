/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_keyring.h"

#include <memory>
#include <utility>

#include "brave/components/brave_wallet/browser/internal/hd_key_ed25519.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "brave/components/brave_wallet/rust/lib.rs.h"
#include "crypto/sha2.h"

namespace brave_wallet {

namespace {

constexpr size_t kMaxSeeds = 16;
constexpr size_t kMaxSeedLen = 32;

}  // namespace

void SolanaKeyring::ConstructRootHDKey(const std::vector<uint8_t>& seed,
                                       const std::string& hd_path) {
  if (!seed.empty()) {
    std::unique_ptr<HDKeyEd25519> hd_key = HDKeyEd25519::GenerateFromSeed(seed);
    master_key_ = std::unique_ptr<HDKeyBase>{hd_key.release()};
    if (master_key_) {
      root_ = master_key_->DeriveChildFromPath(hd_path);
    }
  }
}

void SolanaKeyring::AddAccounts(size_t number) {
  size_t cur_accounts_number = accounts_.size();
  for (size_t i = cur_accounts_number; i < cur_accounts_number + number; ++i) {
    if (root_) {
      accounts_.push_back(root_->DeriveChild(i)->DeriveChild(0));
    }
  }
}

std::string SolanaKeyring::ImportAccount(const std::vector<uint8_t>& keypair) {
  // extract private key from keypair
  std::vector<uint8_t> private_key = std::vector<uint8_t>(
      keypair.begin(), keypair.begin() + kSolanaPrikeySize);
  std::unique_ptr<HDKeyEd25519> hd_key =
      HDKeyEd25519::GenerateFromPrivateKey(private_key);
  if (!hd_key)
    return std::string();

  const std::string address = GetAddressInternal(hd_key.get());
  if (!AddImportedAddress(address, std::move(hd_key))) {
    return std::string();
  }

  return address;
}

std::string SolanaKeyring::GetAddressInternal(HDKeyBase* hd_key_base) const {
  if (!hd_key_base)
    return std::string();
  HDKeyEd25519* hd_key = static_cast<HDKeyEd25519*>(hd_key_base);
  return hd_key->GetBase58EncodedPublicKey();
}

// Create a valid program derived address without searching for a bump seed.
// https://docs.rs/solana-program/latest/solana_program/pubkey/struct.Pubkey.html#method.create_program_address
// static
absl::optional<std::string> SolanaKeyring::CreateProgramDerivedAddress(
    const std::vector<std::vector<uint8_t>>& seeds,
    const std::string& program_id) {
  const std::string pda_marker = "ProgramDerivedAddress";

  std::vector<uint8_t> program_id_bytes;
  if (!Base58Decode(program_id, &program_id_bytes, kSolanaPubkeySize))
    return absl::nullopt;

  if (seeds.size() > kMaxSeeds) {
    return absl::nullopt;
  }

  for (const auto& seed : seeds) {
    if (seed.size() > kMaxSeedLen) {
      return absl::nullopt;
    }
  }

  std::vector<uint8_t> buffer;
  for (const auto& seed : seeds) {
    buffer.insert(buffer.end(), seed.begin(), seed.end());
  }

  buffer.insert(buffer.end(), program_id_bytes.begin(), program_id_bytes.end());
  buffer.insert(buffer.end(), pda_marker.begin(), pda_marker.end());

  auto hash_array = crypto::SHA256Hash(buffer);
  std::vector<uint8_t> hash_vec(hash_array.begin(), hash_array.end());

  // Invalid because program derived addresses have to be off-curve.
  if (bytes_are_curve25519_point(
          rust::Slice<const uint8_t>{hash_vec.data(), hash_vec.size()})) {
    return absl::nullopt;
  }

  return Base58Encode(hash_vec);
}

// Find a valid program derived address and its corresponding bump seed.
// https://docs.rs/solana-program/latest/solana_program/pubkey/struct.Pubkey.html#method.find_program_address
// static
absl::optional<std::string> SolanaKeyring::FindProgramDerivedAddress(
    const std::vector<std::vector<uint8_t>>& seeds,
    const std::string& program_id,
    uint8_t* ret_bump_seed) {
  std::vector<std::vector<uint8_t>> seeds_with_bump(seeds);
  std::vector<uint8_t> bump_seed = {UINT8_MAX};
  while (bump_seed[0] != 0) {
    seeds_with_bump.push_back(bump_seed);

    auto address = CreateProgramDerivedAddress(seeds_with_bump, program_id);
    if (address) {
      if (ret_bump_seed)
        *ret_bump_seed = bump_seed[0];

      return address;
    }

    seeds_with_bump.pop_back();
    --bump_seed[0];
  }

  return absl::nullopt;
}

// Derives the associated token account address for the given wallet address and
// token mint.
// https://docs.rs/spl-associated-token-account/1.0.3/spl_associated_token_account/fn.get_associated_token_address.html
// static
absl::optional<std::string> SolanaKeyring::GetAssociatedTokenAccount(
    const std::string& spl_token_mint_address,
    const std::string& wallet_address) {
  std::vector<std::vector<uint8_t>> seeds;
  std::vector<uint8_t> wallet_address_bytes;
  std::vector<uint8_t> token_program_id_bytes;
  std::vector<uint8_t> spl_token_mint_address_bytes;

  if (!Base58Decode(wallet_address, &wallet_address_bytes, kSolanaPubkeySize) ||
      !Base58Decode(kSolanaTokenProgramId, &token_program_id_bytes,
                    kSolanaPubkeySize) ||
      !Base58Decode(spl_token_mint_address, &spl_token_mint_address_bytes,
                    kSolanaPubkeySize)) {
    return absl::nullopt;
  }

  seeds.push_back(std::move(wallet_address_bytes));
  seeds.push_back(std::move(token_program_id_bytes));
  seeds.push_back(std::move(spl_token_mint_address_bytes));

  return FindProgramDerivedAddress(seeds, kSolanaAssociatedTokenProgramId);
}

}  // namespace brave_wallet
