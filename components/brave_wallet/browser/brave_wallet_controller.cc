/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_controller.h"

#include <utility>

#include "brave/common/brave_wallet_constants.h"
#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "brave/common/extensions/extension_constants.h"
#include "content/public/browser/browser_context.h"
#include "base/base64.h"
#include "crypto/aead.h"
#include "crypto/hkdf.h"
#include "crypto/random.h"
#include "crypto/symmetric_key.h"
#include "brave/common/pref_names.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/shared_user_script_master.h"
#include "extensions/browser/unloaded_extension_reason.h"

namespace {
  bool ResetCryptoWalletsOnFileTaskRunner(
      const base::FilePath& path) {
    return base::DeleteFile(path, true);
  }
}

BraveWalletController::BraveWalletController(content::BrowserContext* context)
    : context_(context), extension_registry_observer_(this),
      file_task_runner_(base::CreateSequencedTaskRunner(
          {base::ThreadPool(), base::MayBlock(),
           base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
    weak_factory_(this) {
  Profile* profile = Profile::FromBrowserContext(context_);
  pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
  pref_change_registrar_->Init(profile->GetPrefs());
  pref_change_registrar_->Add(kBraveWalletWeb3Provider,
    base::Bind(&BraveWalletController::OnPreferenceChanged,
        base::Unretained(this)));
  // In case any web3 providers have already loaded content scripts at
  // this point.
  RemoveUnusedWeb3ProviderContentScripts();
  extension_registry_observer_.Add(extensions::ExtensionRegistry::Get(context));
}

BraveWalletController::~BraveWalletController() {
}

const size_t BraveWalletController::kNonceByteLength = 12;
const size_t BraveWalletController::kSeedByteLength = 32;

// Returns 32 bytes of output from HKDF-SHA256.
// This is done so that BitGo never actually directly has
// access to the master seed, but it does have a deterministic seed.
// The salt value is the same intentionally on all clients.
// See https://github.com/brave/brave-browser/wiki/Brave-Ethereum-Remote-Client-Wallet-Seed-Information#note-on-salts
// static
std::string
BraveWalletController::GetBitGoSeedFromRootSeed(
    const std::string& seed) {
  base::StringPiece salt("brave-bitgo-salt");
  base::StringPiece info("bitgo");
  return crypto::HkdfSha256(base::StringPiece(seed.begin(), seed.end()),
      salt, info, kSeedByteLength);
}

// Returns 32 bytes of output from HKDF-SHA256.
// This is done so that ethereum-remote-client never actually directly has
// access to the master seed, but it does have a deterministic seed.
// The salt value is the same intentionally on all clients.
// See https://github.com/brave/brave-browser/wiki/Brave-Ethereum-Remote-Client-Wallet-Seed-Information#note-on-salts
// static
std::string
BraveWalletController::GetEthereumRemoteClientSeedFromRootSeed(
    const std::string& seed) {
  base::StringPiece salt("brave-ethwallet-salt");
  base::StringPiece info("ethwallet");
  return crypto::HkdfSha256(base::StringPiece(seed.begin(), seed.end()),
      salt, info, kSeedByteLength);
}

// static
bool BraveWalletController::LoadFromPrefs(
    Profile* profile,
    std::string* cipher_seed, std::string* nonce) {
  if (!profile->GetPrefs()->HasPrefPath(kBraveWalletAES256GCMSivNonce) ||
      !profile->GetPrefs()->HasPrefPath(kBraveWalletEncryptedSeed)) {
    return false;
  }
  if (!base::Base64Decode(
          profile->GetPrefs()->GetString(kBraveWalletAES256GCMSivNonce),
          nonce)) {
    return false;
  }
  if (!base::Base64Decode(
          profile->GetPrefs()->GetString(kBraveWalletEncryptedSeed),
          cipher_seed)) {
    return false;
  }
  return true;
}

// static
bool BraveWalletController::OpenSeed(const std::string& cipher_seed,
    const std::string& key, const std::string& nonce,
    std::string* seed) {
  crypto::Aead aes_256_gcm_siv(crypto::Aead::AES_256_GCM_SIV);
  aes_256_gcm_siv.Init(&key);
  return aes_256_gcm_siv.Open(cipher_seed, nonce, base::StringPiece(""), seed);
}

// Generate a new random nonce
// static
std::string BraveWalletController::GetRandomNonce() {
  // crypto::RandBytes is fail safe.
  uint8_t nonceBytes[kNonceByteLength];
  crypto::RandBytes(nonceBytes, kNonceByteLength);
  return std::string(
      reinterpret_cast<char*>(nonceBytes), kNonceByteLength);
}

// static
std::string BraveWalletController::GetRandomSeed() {
  // crypto::RandBytes is fail safe.
  uint8_t random_seed_bytes[kSeedByteLength];
  crypto::RandBytes(random_seed_bytes, kSeedByteLength);
  return std::string(
      reinterpret_cast<char*>(random_seed_bytes), kSeedByteLength);
}

// static
bool BraveWalletController::SealSeed(const std::string& seed,
    const std::string& key, const std::string& nonce,
    std::string* cipher_seed) {
  crypto::Aead aes_256_gcm_siv(crypto::Aead::AES_256_GCM_SIV);
  aes_256_gcm_siv.Init(&key);
  return aes_256_gcm_siv.Seal(base::StringPiece(seed.begin(), seed.end()),
      nonce, base::StringPiece(""), cipher_seed);
}

// Store the seed in preferences, binary pref strings need to be
// base64 encoded.  Base64 encoding is fail safe.
// static
void BraveWalletController::SaveToPrefs(
    Profile* profile,
    const std::string& cipher_seed,
    const std::string& nonce) {
  // Store the seed in preferences, binary pref strings need to be
  // base64 encoded.  Base64 encoding is fail safe.
  std::string base64_nonce;
  std::string base64_cipher_seed;
  base::Base64Encode(nonce, &base64_nonce);
  base::Base64Encode(base::StringPiece(cipher_seed.begin(),
                                      cipher_seed.end()),
      &base64_cipher_seed);
  profile->GetPrefs()->SetString(kBraveWalletAES256GCMSivNonce, base64_nonce);
  profile->GetPrefs()->SetString(kBraveWalletEncryptedSeed,
      base64_cipher_seed);
}

void BraveWalletController::ResetCryptoWallets() {
  Profile* profile = Profile::FromBrowserContext(context_);
  const base::FilePath wallet_data_path = profile->GetPath()
      .AppendASCII("Local Extension Settings")
      .AppendASCII(ethereum_remote_client_extension_id);

  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&ResetCryptoWalletsOnFileTaskRunner, wallet_data_path),
      base::BindOnce(&BraveWalletController::OnCryptoWalletsReset,
                     weak_factory_.GetWeakPtr()));
}

void BraveWalletController::OnCryptoWalletsReset(bool success) {
  if (success) {
    BraveWalletController::CloseTabsAndRestart();
  }
}

void BraveWalletController::CloseTabsAndRestart() {
  // Close all CW tabs in each browser instance
  for (auto* browser : *BrowserList::GetInstance()) {
    auto* tab_strip = browser->tab_strip_model();
    for (int i = 0; i < tab_strip->count(); ++i) {
      auto* web_contents = tab_strip->GetWebContentsAt(i);
      GURL url = web_contents->GetURL();
      if (url.SchemeIs(content::kChromeUIScheme) &&
          url.host() == ethereum_remote_client_host) {
        web_contents->Close();
      }
    }
  }
  chrome::AttemptRestart();
}

// Generates a random 32 byte root seed and stores it in prefs
// in an encrypted form.  It also stores the nonce that was used
// from AES 256 GCM SIV.
// If this function is called multiple times, the previous value
// from prefs will be re-used.
// The return value will be true if successful
// https://github.com/brave/brave-browser/wiki/Brave-Ethereum-Remote-Client-Wallet-Seed-Information
bool BraveWalletController::LoadRootSeedInfo(std::vector<uint8_t> key,
    std::string* seed) {
  std::string nonce;
  std::string cipher_seed;
  if (!seed) {
    return false;
  }
  Profile* profile = Profile::FromBrowserContext(context_);
  // Check if we already have a nonce and seed stored in prefs.
  std::string aes_256_gcm_siv_key(key.begin(), key.end());
  if (BraveWalletController::LoadFromPrefs(
          profile, &cipher_seed, &nonce)) {
    // Decrypt the existing seed.
    if (!BraveWalletController::OpenSeed(
            cipher_seed, aes_256_gcm_siv_key, nonce, seed)) {
      return false;
    }
  } else {
    // No valid previous value was stored, so generate new random values.
    nonce = BraveWalletController::GetRandomNonce();
    *seed = BraveWalletController::GetRandomSeed();
    // Encrypt that seed.
    if (!BraveWalletController::SealSeed(
            *seed, aes_256_gcm_siv_key, nonce, &cipher_seed)) {
      return false;
    }
    // Save it to prefs.
    BraveWalletController::SaveToPrefs(profile, cipher_seed, nonce);
  }
  // We should have the correct nonce size and seed size at this point
  // regardless of if it was newly genearted or retrieved from prefs.
  DCHECK_EQ(nonce.size(), BraveWalletController::kNonceByteLength);
  DCHECK_EQ(seed->size(), BraveWalletController::kSeedByteLength);
  return true;
}

// The return value is passed to chrome.braveWallet.getWalletSeed
// via the second paramter callback function.
// The return value will not be the root seed, but instead a
// deterministic hash of that seed with HKDF, so that we can use
// other HKDF hashes with different info parameters for different purposes.
// For more information, see:
// https://github.com/brave/brave-browser/wiki/Brave-Ethereum-Remote-Client-Wallet-Seed-Information
std::string BraveWalletController::GetWalletSeed(
    std::vector<uint8_t> key) {
  std::string seed;
  if (!LoadRootSeedInfo(key, &seed)) {
    return "";
  }
  return BraveWalletController::GetEthereumRemoteClientSeedFromRootSeed(
      seed);
}

// The return value is passed to chrome.braveWallet.getBitGoSeed
// via the second paramter callback function.
// The return value will not be the root seed, but instead a
// deterministic hash of that seed with HKDF, so that we can use
// other HKDF hashes with different info parameters for different purposes.
// For more information, see:
// https://github.com/brave/brave-browser/wiki/Brave-Ethereum-Remote-Client-Wallet-Seed-Information
std::string BraveWalletController::GetBitGoSeed(
    std::vector<uint8_t> key) {
  std::string seed;
  if (!LoadRootSeedInfo(key, &seed)) {
    return "";
  }
  return BraveWalletController::GetBitGoSeedFromRootSeed(seed);
}

void BraveWalletController::RemoveUnusedWeb3ProviderContentScripts() {
  Profile* profile = Profile::FromBrowserContext(context_);
  auto* shared_user_script_master =
      extensions::ExtensionSystem::Get(context_)->shared_user_script_master();
  auto* registry = extensions::ExtensionRegistry::Get(profile);
  auto* metamask_extension =
      registry->enabled_extensions().GetByID(metamask_extension_id);
  auto* erc_extension =
      registry->enabled_extensions().GetByID(
          ethereum_remote_client_extension_id);
  auto provider = static_cast<BraveWalletWeb3ProviderTypes>(
      profile->GetPrefs()->GetInteger(kBraveWalletWeb3Provider));
  if (metamask_extension) {
    shared_user_script_master->OnExtensionUnloaded(
        context_, metamask_extension,
        extensions::UnloadedExtensionReason::DISABLE);
  }
  if (erc_extension) {
    shared_user_script_master->OnExtensionUnloaded(
        context_, erc_extension,
        extensions::UnloadedExtensionReason::DISABLE);
  }
  if (provider == BraveWalletWeb3ProviderTypes::CRYPTO_WALLETS) {
    if (erc_extension) {
      shared_user_script_master->OnExtensionLoaded(context_, erc_extension);
    }
  } else if (provider == BraveWalletWeb3ProviderTypes::METAMASK) {
    if (metamask_extension) {
      shared_user_script_master->OnExtensionLoaded(
          context_, metamask_extension);
    }
  }
}

void BraveWalletController::OnPreferenceChanged() {
  RemoveUnusedWeb3ProviderContentScripts();
}

void BraveWalletController::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension) {
  RemoveUnusedWeb3ProviderContentScripts();
}
