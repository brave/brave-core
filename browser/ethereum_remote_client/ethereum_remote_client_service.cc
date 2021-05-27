/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ethereum_remote_client/ethereum_remote_client_service.h"

#include <string>
#include <utility>

#include "base/base64.h"
#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/task_runner_util.h"
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_constants.h"
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_delegate.h"
#include "brave/browser/ethereum_remote_client/pref_names.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "crypto/aead.h"
#include "crypto/hkdf.h"
#include "crypto/random.h"
#include "crypto/symmetric_key.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/unloaded_extension_reason.h"
#include "extensions/browser/user_script_manager.h"
#endif

EthereumRemoteClientService::EthereumRemoteClientService(
    content::BrowserContext* context,
    std::unique_ptr<EthereumRemoteClientDelegate>
        ethereum_remote_client_delegate)
    : context_(context),
      ethereum_remote_client_delegate_(
          std::move(ethereum_remote_client_delegate)),
#if BUILDFLAG(ENABLE_EXTENSIONS)
      extension_registry_observer_(this),
#endif
      file_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})) {
  pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
  pref_change_registrar_->Init(user_prefs::UserPrefs::Get(context_));
  pref_change_registrar_->Add(
      kBraveWalletWeb3Provider,
      base::Bind(&EthereumRemoteClientService::OnPreferenceChanged,
                 base::Unretained(this)));
  // In case any web3 providers have already loaded content scripts at
  // this point.
  RemoveUnusedWeb3ProviderContentScripts();
#if BUILDFLAG(ENABLE_EXTENSIONS)
  extension_registry_observer_.Add(extensions::ExtensionRegistry::Get(context));
#endif
}

EthereumRemoteClientService::~EthereumRemoteClientService() {}

const size_t EthereumRemoteClientService::kNonceByteLength = 12;
const size_t EthereumRemoteClientService::kSeedByteLength = 32;

// Returns 32 bytes of output from HKDF-SHA256.
// This is done so that BitGo never actually directly has
// access to the master seed, but it does have a deterministic seed.
// The salt value is the same intentionally on all clients.
// See
// https://github.com/brave/brave-browser/wiki/Brave-Ethereum-Remote-Client-Wallet-Seed-Information#note-on-salts
// static
std::string EthereumRemoteClientService::GetBitGoSeedFromRootSeed(
    const std::string& seed) {
  base::StringPiece salt("brave-bitgo-salt");
  base::StringPiece info("bitgo");
  return crypto::HkdfSha256(base::MakeStringPiece(seed.begin(), seed.end()),
                            salt, info, kSeedByteLength);
}

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
  base::StringPiece salt("brave-ethwallet-salt");
  base::StringPiece info("ethwallet");
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
  return aes_256_gcm_siv.Open(cipher_seed, nonce, base::StringPiece(""), seed);
}

// Generate a new random nonce
// static
std::string EthereumRemoteClientService::GetRandomNonce() {
  // crypto::RandBytes is fail safe.
  uint8_t nonceBytes[kNonceByteLength];
  crypto::RandBytes(nonceBytes, kNonceByteLength);
  return std::string(reinterpret_cast<char*>(nonceBytes), kNonceByteLength);
}

// static
std::string EthereumRemoteClientService::GetRandomSeed() {
  // crypto::RandBytes is fail safe.
  uint8_t random_seed_bytes[kSeedByteLength];
  crypto::RandBytes(random_seed_bytes, kSeedByteLength);
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
                              nonce, base::StringPiece(""), cipher_seed);
}

// Store the seed in preferences, binary pref strings need to be
// base64 encoded.  Base64 encoding is fail safe.
// static
void EthereumRemoteClientService::SaveToPrefs(PrefService* prefs,
                                              const std::string& cipher_seed,
                                              const std::string& nonce) {
  // Store the seed in preferences, binary pref strings need to be
  // base64 encoded.  Base64 encoding is fail safe.
  std::string base64_nonce;
  std::string base64_cipher_seed;
  base::Base64Encode(nonce, &base64_nonce);
  base::Base64Encode(
      base::MakeStringPiece(cipher_seed.begin(), cipher_seed.end()),
      &base64_cipher_seed);
  prefs->SetString(kERCAES256GCMSivNonce, base64_nonce);
  prefs->SetString(kERCEncryptedSeed, base64_cipher_seed);
}

void EthereumRemoteClientService::ResetCryptoWallets() {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  extensions::ExtensionPrefs::Get(context_)->DeleteExtensionPrefs(
      ethereum_remote_client_extension_id);
#endif
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

// The return value is passed to chrome.braveWallet.getBitGoSeed
// via the second paramter callback function.
// The return value will not be the root seed, but instead a
// deterministic hash of that seed with HKDF, so that we can use
// other HKDF hashes with different info parameters for different purposes.
// For more information, see:
// https://github.com/brave/brave-browser/wiki/Brave-Ethereum-Remote-Client-Wallet-Seed-Information
std::string EthereumRemoteClientService::GetBitGoSeed(
    std::vector<uint8_t> key) {
  std::string seed;
  if (!LoadRootSeedInfo(key, &seed)) {
    return "";
  }
  return EthereumRemoteClientService::GetBitGoSeedFromRootSeed(seed);
}

void EthereumRemoteClientService::RemoveUnusedWeb3ProviderContentScripts() {
// We don't use ExtensionRegistryObserver and simply access the private methods
// OnExtensionLoaded()/OnExtensionUnloaded() from UserScriptLoader instead since
// we only want to load/unload the content scripts and not the extension.
#if BUILDFLAG(ENABLE_EXTENSIONS)
  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  auto* user_script_manager =
      extensions::ExtensionSystem::Get(context_)->user_script_manager();
  if (!user_script_manager) {
    return;
  }
  auto* registry = extensions::ExtensionRegistry::Get(context_);
  auto provider = static_cast<brave_wallet::Web3ProviderTypes>(
      prefs->GetInteger(kBraveWalletWeb3Provider));

  auto* erc_extension = registry->enabled_extensions().GetByID(
      ethereum_remote_client_extension_id);
  if (erc_extension) {
    user_script_manager->OnExtensionUnloaded(
        context_, erc_extension, extensions::UnloadedExtensionReason::DISABLE);
  }
  auto* metamask_extension =
      registry->enabled_extensions().GetByID(metamask_extension_id);
  if (metamask_extension) {
    user_script_manager->OnExtensionUnloaded(
        context_, metamask_extension,
        extensions::UnloadedExtensionReason::DISABLE);
  }

  // If the user has not manually gone into settings and selected
  // they want to use Crypto Wallets. Then we prefer MetaMask.
  // MetaMask is the default if it is installed.
  // We can't have 2 web3 providers, we:
  // 1) Check if MetaMask content scripts are disabled, if so, enable them.
  // 2) Check if CryptoWallets content scripts are enabled, if so, disable them.
  if (provider == brave_wallet::Web3ProviderTypes::CRYPTO_WALLETS) {
    if (erc_extension) {
      user_script_manager->OnExtensionLoaded(context_, erc_extension);
    }
  } else if (provider != brave_wallet::Web3ProviderTypes::NONE) {
    if (metamask_extension) {
      user_script_manager->OnExtensionLoaded(context_, metamask_extension);
    }
  }
#endif
}

void EthereumRemoteClientService::OnPreferenceChanged() {
  RemoveUnusedWeb3ProviderContentScripts();
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
void EthereumRemoteClientService::OnExtensionInstalled(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    bool is_update) {
  if (extension->id() == metamask_extension_id && !is_update) {
    PrefService* prefs = user_prefs::UserPrefs::Get(context_);
    prefs->SetInteger(
        kBraveWalletWeb3Provider,
        static_cast<int>(brave_wallet::Web3ProviderTypes::METAMASK));
    RemoveUnusedWeb3ProviderContentScripts();
  }
}

void EthereumRemoteClientService::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension) {
  if (extension->id() == metamask_extension_id ||
      extension->id() == ethereum_remote_client_extension_id) {
    RemoveUnusedWeb3ProviderContentScripts();
  }
}

void EthereumRemoteClientService::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    extensions::UnloadedExtensionReason reason) {
  if (extension->id() == metamask_extension_id ||
      extension->id() == ethereum_remote_client_extension_id) {
    RemoveUnusedWeb3ProviderContentScripts();
  }
}

void EthereumRemoteClientService::OnExtensionUninstalled(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    extensions::UninstallReason reason) {
  if (extension->id() == metamask_extension_id) {
    PrefService* prefs = user_prefs::UserPrefs::Get(context_);
    auto provider = static_cast<brave_wallet::Web3ProviderTypes>(
        prefs->GetInteger(kBraveWalletWeb3Provider));
    if (provider == brave_wallet::Web3ProviderTypes::METAMASK)
      prefs->SetInteger(
          kBraveWalletWeb3Provider,
          static_cast<int>(
              brave_wallet::IsNativeWalletEnabled()
                  ? brave_wallet::Web3ProviderTypes::BRAVE_WALLET
                  : brave_wallet::Web3ProviderTypes::CRYPTO_WALLETS));
    RemoveUnusedWeb3ProviderContentScripts();
  }
}

void EthereumRemoteClientService::CryptoWalletsExtensionReady() {
  if (load_ui_callback_) {
    std::move(load_ui_callback_).Run();
  }
}
#endif

bool EthereumRemoteClientService::IsCryptoWalletsSetup() const {
  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  return prefs->HasPrefPath(kERCAES256GCMSivNonce) &&
         prefs->HasPrefPath(kERCEncryptedSeed);
}

bool EthereumRemoteClientService::IsCryptoWalletsReady() const {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  auto* registry = extensions::ExtensionRegistry::Get(context_);
  return registry->ready_extensions().Contains(
      ethereum_remote_client_extension_id);
#else
  return true;
#endif
}

bool EthereumRemoteClientService::ShouldShowLazyLoadInfobar() const {
  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  auto provider = static_cast<brave_wallet::Web3ProviderTypes>(
      prefs->GetInteger(kBraveWalletWeb3Provider));
  return provider == brave_wallet::Web3ProviderTypes::CRYPTO_WALLETS &&
         !IsCryptoWalletsReady();
}

void EthereumRemoteClientService::MaybeLoadCryptoWalletsExtension(
    LoadUICallback callback) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  load_ui_callback_ = std::move(callback);
  ethereum_remote_client_delegate_->MaybeLoadCryptoWalletsExtension(context_);
#endif
}
