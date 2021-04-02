/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"

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
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_delegate.h"
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

BraveWalletService::BraveWalletService(
    content::BrowserContext* context,
    std::unique_ptr<BraveWalletDelegate> brave_wallet_delegate)
    : context_(context),
      brave_wallet_delegate_(std::move(brave_wallet_delegate)),
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
      base::Bind(&BraveWalletService::OnPreferenceChanged,
                 base::Unretained(this)));
  // In case any web3 providers have already loaded content scripts at
  // this point.
  RemoveUnusedWeb3ProviderContentScripts();
#if BUILDFLAG(ENABLE_EXTENSIONS)
  extension_registry_observer_.Add(extensions::ExtensionRegistry::Get(context));
#endif

  controller_ = std::make_unique<brave_wallet::EthJsonRpcController>(
      context, brave_wallet::Network::kMainnet);
}

BraveWalletService::~BraveWalletService() {}

const size_t BraveWalletService::kNonceByteLength = 12;
const size_t BraveWalletService::kSeedByteLength = 32;

// Returns 32 bytes of output from HKDF-SHA256.
// This is done so that BitGo never actually directly has
// access to the master seed, but it does have a deterministic seed.
// The salt value is the same intentionally on all clients.
// See
// https://github.com/brave/brave-browser/wiki/Brave-Ethereum-Remote-Client-Wallet-Seed-Information#note-on-salts
// static
std::string BraveWalletService::GetBitGoSeedFromRootSeed(
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
std::string BraveWalletService::GetEthereumRemoteClientSeedFromRootSeed(
    const std::string& seed) {
  base::StringPiece salt("brave-ethwallet-salt");
  base::StringPiece info("ethwallet");
  return crypto::HkdfSha256(base::MakeStringPiece(seed.begin(), seed.end()),
                            salt, info, kSeedByteLength);
}

// static
bool BraveWalletService::LoadFromPrefs(PrefService* prefs,
                                       std::string* cipher_seed,
                                       std::string* nonce) {
  if (!prefs->HasPrefPath(kBraveWalletAES256GCMSivNonce) ||
      !prefs->HasPrefPath(kBraveWalletEncryptedSeed)) {
    return false;
  }
  if (!base::Base64Decode(prefs->GetString(kBraveWalletAES256GCMSivNonce),
                          nonce)) {
    return false;
  }
  if (!base::Base64Decode(prefs->GetString(kBraveWalletEncryptedSeed),
                          cipher_seed)) {
    return false;
  }
  return true;
}

// static
bool BraveWalletService::OpenSeed(const std::string& cipher_seed,
                                  const std::string& key,
                                  const std::string& nonce,
                                  std::string* seed) {
  crypto::Aead aes_256_gcm_siv(crypto::Aead::AES_256_GCM_SIV);
  aes_256_gcm_siv.Init(&key);
  return aes_256_gcm_siv.Open(cipher_seed, nonce, base::StringPiece(""), seed);
}

// Generate a new random nonce
// static
std::string BraveWalletService::GetRandomNonce() {
  // crypto::RandBytes is fail safe.
  uint8_t nonceBytes[kNonceByteLength];
  crypto::RandBytes(nonceBytes, kNonceByteLength);
  return std::string(reinterpret_cast<char*>(nonceBytes), kNonceByteLength);
}

// static
std::string BraveWalletService::GetRandomSeed() {
  // crypto::RandBytes is fail safe.
  uint8_t random_seed_bytes[kSeedByteLength];
  crypto::RandBytes(random_seed_bytes, kSeedByteLength);
  return std::string(reinterpret_cast<char*>(random_seed_bytes),
                     kSeedByteLength);
}

// static
bool BraveWalletService::SealSeed(const std::string& seed,
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
void BraveWalletService::SaveToPrefs(PrefService* prefs,
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
  prefs->SetString(kBraveWalletAES256GCMSivNonce, base64_nonce);
  prefs->SetString(kBraveWalletEncryptedSeed, base64_cipher_seed);
}

void BraveWalletService::ResetCryptoWallets() {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  extensions::ExtensionPrefs::Get(context_)->DeleteExtensionPrefs(
      ethereum_remote_client_extension_id);
#endif
}

brave_wallet::EthJsonRpcController* BraveWalletService::controller() const {
  return controller_.get();
}

// Generates a random 32 byte root seed and stores it in prefs
// in an encrypted form.  It also stores the nonce that was used
// from AES 256 GCM SIV.
// If this function is called multiple times, the previous value
// from prefs will be re-used.
// The return value will be true if successful
// https://github.com/brave/brave-browser/wiki/Brave-Ethereum-Remote-Client-Wallet-Seed-Information
bool BraveWalletService::LoadRootSeedInfo(std::vector<uint8_t> key,
                                          std::string* seed) {
  std::string nonce;
  std::string cipher_seed;
  if (!seed) {
    return false;
  }

  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  // Check if we already have a nonce and seed stored in prefs.
  std::string aes_256_gcm_siv_key(key.begin(), key.end());
  if (BraveWalletService::LoadFromPrefs(prefs, &cipher_seed, &nonce)) {
    // Decrypt the existing seed.
    if (!BraveWalletService::OpenSeed(cipher_seed, aes_256_gcm_siv_key, nonce,
                                      seed)) {
      return false;
    }
  } else {
    // No valid previous value was stored, so generate new random values.
    nonce = BraveWalletService::GetRandomNonce();
    *seed = BraveWalletService::GetRandomSeed();
    // Encrypt that seed.
    if (!BraveWalletService::SealSeed(*seed, aes_256_gcm_siv_key, nonce,
                                      &cipher_seed)) {
      return false;
    }
    // Save it to prefs.
    BraveWalletService::SaveToPrefs(prefs, cipher_seed, nonce);
  }
  // We should have the correct nonce size and seed size at this point
  // regardless of if it was newly genearted or retrieved from prefs.
  DCHECK_EQ(nonce.size(), BraveWalletService::kNonceByteLength);
  DCHECK_EQ(seed->size(), BraveWalletService::kSeedByteLength);
  return true;
}

// The return value is passed to chrome.braveWallet.getWalletSeed
// via the second paramter callback function.
// The return value will not be the root seed, but instead a
// deterministic hash of that seed with HKDF, so that we can use
// other HKDF hashes with different info parameters for different purposes.
// For more information, see:
// https://github.com/brave/brave-browser/wiki/Brave-Ethereum-Remote-Client-Wallet-Seed-Information
std::string BraveWalletService::GetWalletSeed(std::vector<uint8_t> key) {
  std::string seed;
  if (!LoadRootSeedInfo(key, &seed)) {
    return "";
  }
  return BraveWalletService::GetEthereumRemoteClientSeedFromRootSeed(seed);
}

// The return value is passed to chrome.braveWallet.getBitGoSeed
// via the second paramter callback function.
// The return value will not be the root seed, but instead a
// deterministic hash of that seed with HKDF, so that we can use
// other HKDF hashes with different info parameters for different purposes.
// For more information, see:
// https://github.com/brave/brave-browser/wiki/Brave-Ethereum-Remote-Client-Wallet-Seed-Information
std::string BraveWalletService::GetBitGoSeed(std::vector<uint8_t> key) {
  std::string seed;
  if (!LoadRootSeedInfo(key, &seed)) {
    return "";
  }
  return BraveWalletService::GetBitGoSeedFromRootSeed(seed);
}

void BraveWalletService::RemoveUnusedWeb3ProviderContentScripts() {
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
  auto provider = static_cast<BraveWalletWeb3ProviderTypes>(
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
  if (provider == BraveWalletWeb3ProviderTypes::CRYPTO_WALLETS) {
    if (erc_extension) {
      user_script_manager->OnExtensionLoaded(context_, erc_extension);
    }
  } else if (provider != BraveWalletWeb3ProviderTypes::NONE) {
    if (metamask_extension) {
      user_script_manager->OnExtensionLoaded(context_, metamask_extension);
    }
  }
#endif
}

void BraveWalletService::OnPreferenceChanged() {
  RemoveUnusedWeb3ProviderContentScripts();
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
void BraveWalletService::OnExtensionInstalled(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    bool is_update) {
  if (extension->id() == metamask_extension_id && !is_update) {
    PrefService* prefs = user_prefs::UserPrefs::Get(context_);
    prefs->SetInteger(kBraveWalletWeb3Provider,
                      static_cast<int>(BraveWalletWeb3ProviderTypes::METAMASK));
    RemoveUnusedWeb3ProviderContentScripts();
  }
}

void BraveWalletService::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension) {
  if (extension->id() == metamask_extension_id ||
      extension->id() == ethereum_remote_client_extension_id) {
    RemoveUnusedWeb3ProviderContentScripts();
  }
}

void BraveWalletService::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    extensions::UnloadedExtensionReason reason) {
  if (extension->id() == metamask_extension_id ||
      extension->id() == ethereum_remote_client_extension_id) {
    RemoveUnusedWeb3ProviderContentScripts();
  }
}

void BraveWalletService::OnExtensionUninstalled(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    extensions::UninstallReason reason) {
  if (extension->id() == metamask_extension_id) {
    PrefService* prefs = user_prefs::UserPrefs::Get(context_);
    auto provider = static_cast<BraveWalletWeb3ProviderTypes>(
        prefs->GetInteger(kBraveWalletWeb3Provider));
    if (provider == BraveWalletWeb3ProviderTypes::METAMASK)
      prefs->SetInteger(
          kBraveWalletWeb3Provider,
          static_cast<int>(brave_wallet::IsNativeWalletEnabled()
                               ? BraveWalletWeb3ProviderTypes::BRAVE_WALLET
                               : BraveWalletWeb3ProviderTypes::CRYPTO_WALLETS));
    RemoveUnusedWeb3ProviderContentScripts();
  }
}

void BraveWalletService::CryptoWalletsExtensionReady() {
  if (load_ui_callback_) {
    std::move(load_ui_callback_).Run();
  }
}
#endif

bool BraveWalletService::IsCryptoWalletsSetup() const {
  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  return prefs->HasPrefPath(kBraveWalletAES256GCMSivNonce) &&
         prefs->HasPrefPath(kBraveWalletEncryptedSeed);
}

bool BraveWalletService::IsCryptoWalletsReady() const {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  auto* registry = extensions::ExtensionRegistry::Get(context_);
  return registry->ready_extensions().Contains(
      ethereum_remote_client_extension_id);
#else
  return true;
#endif
}

bool BraveWalletService::ShouldShowLazyLoadInfobar() const {
  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  auto provider = static_cast<BraveWalletWeb3ProviderTypes>(
      prefs->GetInteger(kBraveWalletWeb3Provider));
  return provider == BraveWalletWeb3ProviderTypes::CRYPTO_WALLETS &&
         !IsCryptoWalletsReady();
}

void BraveWalletService::MaybeLoadCryptoWalletsExtension(
    LoadUICallback callback) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  load_ui_callback_ = std::move(callback);
  brave_wallet_delegate_->MaybeLoadCryptoWalletsExtension(context_);
#endif
}
