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

ExtensionFunction::ResponseAction
BraveWalletPromptToEnableWalletFunction::Run() {
  std::unique_ptr<brave_wallet::PromptToEnableWallet::Params> params(
      brave_wallet::PromptToEnableWallet::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

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
  bool enabled =
    registry->ready_extensions().GetByID(ethereum_remote_client_extension_id);
  return RespondNow(OneArgument(std::make_unique<base::Value>(enabled)));
}

ExtensionFunction::ResponseAction
BraveWalletGetWalletSeedFunction::Run() {
  // Setup AES 256 GCM SIV using the passed in 32-byte key
  std::unique_ptr<brave_wallet::GetWalletSeed::Params> params(
    brave_wallet::GetWalletSeed::Params::Create(*args_));
  if (params->key.size() != 32) {
    return RespondNow(Error("Invalid input key size"));
  }
  std::string aes_256_gcm_siv_key(params->key.begin(), params->key.end());
  crypto::Aead aes_256_gcm_siv(crypto::Aead::AES_256_GCM_SIV);
  aes_256_gcm_siv.Init(&aes_256_gcm_siv_key);

  // Check if we already have a nonce for the algorithm stored,
  // if we do, use that since we'll simply decrypt the seed in that case.
  Profile* profile = Profile::FromBrowserContext(browser_context());
  base::StringPiece aes_256_gcm_siv_nonce;
  const size_t kNonceByteLength = 12;
  if (profile->GetPrefs()->HasPrefPath(kBraveWalletAES256GCMSivNonce)) {
    std::string nonce;
    if (!base::Base64Decode(
             profile->GetPrefs()->GetString(kBraveWalletAES256GCMSivNonce),
             &nonce)) {
      return RespondNow(Error("Error base64 decoding nonce"));
    }
    aes_256_gcm_siv_nonce = nonce;
  } else {
    uint8_t nonceBytes[kNonceByteLength];
    // crypto::RandBytes is fail safe.
    crypto::RandBytes(nonceBytes, kNonceByteLength);
    aes_256_gcm_siv_nonce = base::StringPiece(
        reinterpret_cast<char*>(nonceBytes), kNonceByteLength);
  }
  DCHECK_EQ(aes_256_gcm_siv_nonce.size(), kNonceByteLength);

  // Check if we already have a seed stored, if so, we will return the hash
  // of that.
  const size_t kSeedByteLength = 32;
  std::string seed;
  std::string encrypted_seed;
  if (profile->GetPrefs()->HasPrefPath(kBraveWalletEncryptedSeed)) {
    if (!base::Base64Decode(
             profile->GetPrefs()->GetString(kBraveWalletEncryptedSeed),
             &encrypted_seed)) {
      return RespondNow(Error("Error base64 decoding encrypted seed"));
    }
    // Decrypt the existing seed.
    if (!aes_256_gcm_siv.Open(encrypted_seed, aes_256_gcm_siv_nonce,
             base::StringPiece(""), &seed)) {
      return RespondNow(Error("Error decrypting encrypted seed"));
    }
  } else {
    // Generate a new seed.
    uint8_t random_seed_bytes[kSeedByteLength];
    // crypto::RandBytes is fail safe.
    crypto::RandBytes(random_seed_bytes, kSeedByteLength);

    // Encrypt that seed.
    if (!aes_256_gcm_siv.Seal(base::StringPiece(
            reinterpret_cast<char*>(random_seed_bytes), kSeedByteLength),
        aes_256_gcm_siv_nonce, base::StringPiece(""), &encrypted_seed)) {
      return RespondNow(Error("Error encrypting"));
    }

    // Store the seed in preferences, binary pref strings need to be
    // base64 encoded.  Base64 encoding is fail safe.
    std::string base64_nonce;
    std::string base64_encrypted_seed;
    base::Base64Encode(aes_256_gcm_siv_nonce, &base64_nonce);
    base::Base64Encode(base::StringPiece(encrypted_seed.begin(),
                                         encrypted_seed.end()),
        &base64_encrypted_seed);
    profile->GetPrefs()->SetString(kBraveWalletAES256GCMSivNonce, base64_nonce);
    profile->GetPrefs()->SetString(kBraveWalletEncryptedSeed,
        base64_encrypted_seed);

    seed.assign(random_seed_bytes, random_seed_bytes + kSeedByteLength);
  }
  DCHECK_EQ(seed.size(), kSeedByteLength);

  // Returns 32 bytes of output from HKDF-Expand-SHA256.
  // This is done so that ethereum-remote-client never actually directly has
  // aaccess to the master seed, but it does have a deterministic seed.
  // The salt value is the same intentionally on all clients.
  base::StringPiece hkdfsha256Salt("brave-ethwallet-salt");
  base::StringPiece hkdfsha256Info("ethwallet");
  std::string derived =
      crypto::HkdfSha256(base::StringPiece(seed.begin(), seed.end()),
          hkdfsha256Salt, hkdfsha256Info, kSeedByteLength);
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
