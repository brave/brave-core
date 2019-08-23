/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_wallet_api.h"

#include <memory>
#include <string>

#include "base/base64.h"
#include "base/environment.h"
#include "brave/browser/brave_wallet/brave_wallet_utils.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/common/extensions/api/brave_wallet.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "crypto/aead.h"
#include "crypto/hkdf.h"
#include "crypto/random.h"
#include "crypto/symmetric_key.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_util.h"

namespace extensions {
namespace api {

const size_t BraveWalletGetWalletSeedFunction::kNonceByteLength = 12;
const size_t BraveWalletGetWalletSeedFunction::kSeedByteLength = 32;

ExtensionFunction::ResponseAction
BraveWalletPromptToEnableWalletFunction::Run() {
  std::unique_ptr<brave_wallet::PromptToEnableWallet::Params> params(
      brave_wallet::PromptToEnableWallet::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (brave::IsTorProfile(profile)) {
    return RespondNow(Error("Not available in Tor profile"));
  }

  // Get web contents for this tab
  content::WebContents* contents = nullptr;
  if (!ExtensionTabUtil::GetTabById(
        params->tab_id,
        Profile::FromBrowserContext(browser_context()),
        include_incognito_information(),
        nullptr,
        nullptr,
        &contents,
        nullptr)) {
    return RespondNow(Error(tabs_constants::kTabNotFoundError,
                            base::NumberToString(params->tab_id)));
  }

  RequestWalletInstallationPermission(contents);
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
BraveWalletIsEnabledFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* registry = extensions::ExtensionRegistry::Get(profile);
  bool enabled = !brave::IsTorProfile(profile) &&
    registry->ready_extensions().GetByID(ethereum_remote_client_extension_id);
  return RespondNow(OneArgument(std::make_unique<base::Value>(enabled)));
}

// Returns 32 bytes of output from HKDF-SHA256.
// This is done so that ethereum-remote-client never actually directly has
// access to the master seed, but it does have a deterministic seed.
// The salt value is the same intentionally on all clients.
// See https://github.com/brave/brave-browser/wiki/Brave-Ethereum-Remote-Client-Wallet-Seed-Information#note-on-salts
std::string
BraveWalletGetWalletSeedFunction::GetEthereumRemoteClientSeedFromRootSeed(
    const std::string& seed) {
  base::StringPiece salt("brave-ethwallet-salt");
  base::StringPiece info("ethwallet");
  return crypto::HkdfSha256(base::StringPiece(seed.begin(), seed.end()),
      salt, info, kSeedByteLength);
}

bool
BraveWalletGetWalletSeedFunction::SealSeed(const std::string& seed,
    const std::string& key, const std::string& nonce,
    std::string* cipher_seed) {
  crypto::Aead aes_256_gcm_siv(crypto::Aead::AES_256_GCM_SIV);
  aes_256_gcm_siv.Init(&key);
  return aes_256_gcm_siv.Seal(base::StringPiece(seed.begin(), seed.end()),
      nonce, base::StringPiece(""), cipher_seed);
}

bool BraveWalletGetWalletSeedFunction::OpenSeed(const std::string& cipher_seed,
    const std::string& key, const std::string& nonce,
    std::string* seed) {
  crypto::Aead aes_256_gcm_siv(crypto::Aead::AES_256_GCM_SIV);
  aes_256_gcm_siv.Init(&key);
  return aes_256_gcm_siv.Open(cipher_seed, nonce, base::StringPiece(""), seed);
}

// Store the seed in preferences, binary pref strings need to be
// base64 encoded.  Base64 encoding is fail safe.
void BraveWalletGetWalletSeedFunction::SaveToPrefs(
    Profile* profile, const std::string& cipher_seed,
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

bool BraveWalletGetWalletSeedFunction::LoadFromPrefs(
    Profile* profile, std::string* cipher_seed, std::string* nonce) {
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

// Generate a new random nonce
std::string BraveWalletGetWalletSeedFunction::GetRandomNonce() {
  // crypto::RandBytes is fail safe.
  uint8_t nonceBytes[kNonceByteLength];
  crypto::RandBytes(nonceBytes, kNonceByteLength);
  return std::string(
      reinterpret_cast<char*>(nonceBytes), kNonceByteLength);
}

// Generate a new seed.
std::string BraveWalletGetWalletSeedFunction::GetRandomSeed() {
  // crypto::RandBytes is fail safe.
  uint8_t random_seed_bytes[kSeedByteLength];
  crypto::RandBytes(random_seed_bytes, kSeedByteLength);
  return std::string(
      reinterpret_cast<char*>(random_seed_bytes), kSeedByteLength);
}

// Generates a random 32 byte root seed and stores it in prefs
// in an encrypted form.  It also stores the nonce that was used
// from AES 256 GCM SIV.
// If this function is called multiple times, the previous value
// from prefs will be re-used.
// The return value is passed to chrome.braveWallet.getWalletSeed
// via the second paramter callback function.
// The return value will not be the root seed, but instead a
// deterministic hash of that seed with HKDF, so that we can use
// other HKDF hashes with different info parameters for different purposes.
// For more information, see:
// https://github.com/brave/brave-browser/wiki/Brave-Ethereum-Remote-Client-Wallet-Seed-Information
ExtensionFunction::ResponseAction
BraveWalletGetWalletSeedFunction::Run() {
  // make sure the passed in enryption key is 32 bytes.
  std::unique_ptr<brave_wallet::GetWalletSeed::Params> params(
    brave_wallet::GetWalletSeed::Params::Create(*args_));
  if (params->key.size() != 32) {
    return RespondNow(Error("Invalid input key size"));
  }

  std::string nonce;
  std::string cipher_seed;
  std::string seed;
  // Check if we already have a nonce and seed stored in prefs.
  std::string aes_256_gcm_siv_key(params->key.begin(), params->key.end());
  if (LoadFromPrefs(Profile::FromBrowserContext(browser_context()),
        &cipher_seed, &nonce)) {
    // Decrypt the existing seed.
    if (!OpenSeed(cipher_seed, aes_256_gcm_siv_key, nonce, &seed)) {
      return RespondNow(Error("Error decrypting cipher seed"));
    }
  } else {
    // No valid previous value was stored, so generate new random values.
    nonce = GetRandomNonce();
    seed = GetRandomSeed();
    // Encrypt that seed.
    if (!SealSeed(seed, aes_256_gcm_siv_key, nonce, &cipher_seed)) {
      return RespondNow(Error("Error encrypting"));
    }
    // Save it to prefs.
    SaveToPrefs(Profile::FromBrowserContext(browser_context()), cipher_seed,
        nonce);
  }
  // We should have the correct nonce size and seed size at this point
  // regardless of if it was newly genearted or retrieved from prefs.
  DCHECK_EQ(nonce.size(), kNonceByteLength);
  DCHECK_EQ(seed.size(), kSeedByteLength);
  std::string derived = GetEthereumRemoteClientSeedFromRootSeed(seed);
  base::Value::BlobStorage blob;
  blob.assign(derived.begin(), derived.end());
  return RespondNow(OneArgument(
      std::make_unique<base::Value>(blob)));
}

ExtensionFunction::ResponseAction
BraveWalletGetProjectIDFunction::Run() {
  std::string project_id(BRAVE_INFURA_PROJECT_ID);
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  if (env->HasVar("BRAVE_INFURA_PROJECT_ID")) {
    env->GetVar("BRAVE_INFURA_PROJECT_ID", &project_id);
  }
  return RespondNow(OneArgument(
      std::make_unique<base::Value>(project_id)));
}

}  // namespace api
}  // namespace extensions
