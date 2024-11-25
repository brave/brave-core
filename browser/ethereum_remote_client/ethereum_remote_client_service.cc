/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ethereum_remote_client/ethereum_remote_client_service.h"

#include <string>
#include <string_view>
#include <utility>

#include "base/base64.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/task/thread_pool.h"
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_constants.h"
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_delegate.h"
#include "brave/browser/ethereum_remote_client/pref_names.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "crypto/aead.h"
#include "crypto/hkdf.h"
#include "crypto/random.h"
#include "crypto/symmetric_key.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/unloaded_extension_reason.h"
#include "extensions/browser/user_script_manager.h"

EthereumRemoteClientService::EthereumRemoteClientService(
    content::BrowserContext* context,
    std::unique_ptr<EthereumRemoteClientDelegate>
        ethereum_remote_client_delegate)
    : context_(context),
      ethereum_remote_client_delegate_(
          std::move(ethereum_remote_client_delegate)),
      file_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})) {}

EthereumRemoteClientService::~EthereumRemoteClientService() = default;

// Returns 32 bytes of output from HKDF-SHA256.
// This is done so that ethereum-remote-client never actually directly has
// access to the master seed, but it does have a deterministic seed.
// The salt value is the same intentionally on all clients.
// See
// https://github.com/brave/brave-browser/wiki/Brave-Ethereum-Remote-Client-Wallet-Seed-Information#note-on-salts
// static
std::string
EthereumRemoteClientService::GetEthereumRemoteClientSeedFromRootSeed(
    const std::string& seed) {
  std::string_view salt("brave-ethwallet-salt");
  std::string_view info("ethwallet");
  return crypto::HkdfSha256(base::MakeStringPiece(seed.begin(), seed.end()),
                            salt, info, kSeedByteLength);
}

// static
bool EthereumRemoteClientService::LoadFromPrefs(PrefService* prefs,
                                                std::string* cipher_seed,
                                                std::string* nonce) {
  if (!prefs->HasPrefPath(kERCAES256GCMSivNonce) ||
      !prefs->HasPrefPath(kERCEncryptedSeed)) {
    return false;
  }
  if (!base::Base64Decode(prefs->GetString(kERCAES256GCMSivNonce), nonce)) {
    return false;
  }
  if (!base::Base64Decode(prefs->GetString(kERCEncryptedSeed), cipher_seed)) {
    return false;
  }
  return true;
}

// static
bool EthereumRemoteClientService::OpenSeed(const std::string& cipher_seed,
                                           const std::string& key,
                                           const std::string& nonce,
                                           std::string* seed) {
  crypto::Aead aes_256_gcm_siv(crypto::Aead::AES_256_GCM_SIV);
  aes_256_gcm_siv.Init(&key);
  return aes_256_gcm_siv.Open(cipher_seed, nonce, std::string_view(""), seed);
}

// Generate a new random nonce
// static
std::string EthereumRemoteClientService::GetRandomNonce() {
  // crypto::RandBytes is fail safe.
  uint8_t nonceBytes[kNonceByteLength];
  crypto::RandBytes(nonceBytes);
  return std::string(reinterpret_cast<char*>(nonceBytes), kNonceByteLength);
}

// static
std::string EthereumRemoteClientService::GetRandomSeed() {
  // crypto::RandBytes is fail safe.
  uint8_t random_seed_bytes[kSeedByteLength];
  crypto::RandBytes(random_seed_bytes);
  return std::string(reinterpret_cast<char*>(random_seed_bytes),
                     kSeedByteLength);
}

// static
bool EthereumRemoteClientService::SealSeed(const std::string& seed,
                                           const std::string& key,
                                           const std::string& nonce,
                                           std::string* cipher_seed) {
  crypto::Aead aes_256_gcm_siv(crypto::Aead::AES_256_GCM_SIV);
  aes_256_gcm_siv.Init(&key);
  return aes_256_gcm_siv.Seal(base::MakeStringPiece(seed.begin(), seed.end()),
                              nonce, std::string_view(""), cipher_seed);
}

// Store the seed in preferences, binary pref strings need to be
// base64 encoded.  Base64 encoding is fail safe.
// static
void EthereumRemoteClientService::SaveToPrefs(PrefService* prefs,
                                              const std::string& cipher_seed,
                                              const std::string& nonce) {
  // Store the seed in preferences, binary pref strings need to be
  // base64 encoded.  Base64 encoding is fail safe.
  prefs->SetString(kERCAES256GCMSivNonce, base::Base64Encode(nonce));
  prefs->SetString(kERCEncryptedSeed, base::Base64Encode(cipher_seed));
}

void EthereumRemoteClientService::ResetCryptoWallets() {
  extensions::ExtensionPrefs::Get(context_)->DeleteExtensionPrefs(
      kEthereumRemoteClientExtensionId);
}

// Generates a random 32 byte root seed and stores it in prefs
// in an encrypted form.  It also stores the nonce that was used
// from AES 256 GCM SIV.
// If this function is called multiple times, the previous value
// from prefs will be re-used.
// The return value will be true if successful
// https://github.com/brave/brave-browser/wiki/Brave-Ethereum-Remote-Client-Wallet-Seed-Information
bool EthereumRemoteClientService::LoadRootSeedInfo(std::vector<uint8_t> key,
                                                   std::string* seed) {
  std::string nonce;
  std::string cipher_seed;
  if (!seed) {
    return false;
  }

  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  // Check if we already have a nonce and seed stored in prefs.
  std::string aes_256_gcm_siv_key(key.begin(), key.end());
  if (EthereumRemoteClientService::LoadFromPrefs(prefs, &cipher_seed, &nonce)) {
    // Decrypt the existing seed.
    if (!EthereumRemoteClientService::OpenSeed(cipher_seed, aes_256_gcm_siv_key,
                                               nonce, seed)) {
      return false;
    }
  } else {
    // No valid previous value was stored, so generate new random values.
    nonce = EthereumRemoteClientService::GetRandomNonce();
    *seed = EthereumRemoteClientService::GetRandomSeed();
    // Encrypt that seed.
    if (!EthereumRemoteClientService::SealSeed(*seed, aes_256_gcm_siv_key,
                                               nonce, &cipher_seed)) {
      return false;
    }
    // Save it to prefs.
    EthereumRemoteClientService::SaveToPrefs(prefs, cipher_seed, nonce);
  }
  // We should have the correct nonce size and seed size at this point
  // regardless of if it was newly genearted or retrieved from prefs.
  DCHECK_EQ(nonce.size(), EthereumRemoteClientService::kNonceByteLength);
  DCHECK_EQ(seed->size(), EthereumRemoteClientService::kSeedByteLength);
  return true;
}

// The return value is passed to chrome.braveWallet.getWalletSeed
// via the second paramter callback function.
// The return value will not be the root seed, but instead a
// deterministic hash of that seed with HKDF, so that we can use
// other HKDF hashes with different info parameters for different purposes.
// For more information, see:
// https://github.com/brave/brave-browser/wiki/Brave-Ethereum-Remote-Client-Wallet-Seed-Information
std::string EthereumRemoteClientService::GetWalletSeed(
    std::vector<uint8_t> key) {
  std::string seed;
  if (!LoadRootSeedInfo(key, &seed)) {
    return "";
  }
  return EthereumRemoteClientService::GetEthereumRemoteClientSeedFromRootSeed(
      seed);
}

void EthereumRemoteClientService::CryptoWalletsExtensionReady() {
  if (load_ui_callback_) {
    std::move(load_ui_callback_).Run();
  }
}

bool EthereumRemoteClientService::IsLegacyCryptoWalletsSetup() const {
  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  return prefs->HasPrefPath(kERCAES256GCMSivNonce) &&
         prefs->HasPrefPath(kERCEncryptedSeed);
}

bool EthereumRemoteClientService::IsCryptoWalletsReady() const {
  auto* registry = extensions::ExtensionRegistry::Get(context_);
  return registry && registry->ready_extensions().Contains(
                         kEthereumRemoteClientExtensionId);
}

void EthereumRemoteClientService::MaybeLoadCryptoWalletsExtension(
    LoadUICallback callback) {
  if (load_ui_callback_) {
    std::move(load_ui_callback_).Run();
  }
  load_ui_callback_ = std::move(callback);
  ethereum_remote_client_delegate_->MaybeLoadCryptoWalletsExtension(context_);
}

void EthereumRemoteClientService::UnloadCryptoWalletsExtension() {
  ethereum_remote_client_delegate_->UnloadCryptoWalletsExtension(context_);
}
