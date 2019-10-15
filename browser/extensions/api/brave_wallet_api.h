/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BRAVE_WALLET_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_BRAVE_WALLET_API_H_

#include <string>

#include "extensions/browser/extension_function.h"

class Profile;

namespace extensions {
namespace api {

class BraveWalletPromptToEnableWalletFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveWallet.promptToEnableWallet", UNKNOWN)

 protected:
  ~BraveWalletPromptToEnableWalletFunction() override {}

  ResponseAction Run() override;
};

class BraveWalletIsEnabledFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveWallet.isEnabled", UNKNOWN)

 protected:
  ~BraveWalletIsEnabledFunction() override {}

  ResponseAction Run() override;
};

class BraveWalletIsInstalledFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveWallet.isInstalled", UNKNOWN)

 protected:
  ~BraveWalletIsInstalledFunction() override {}

  ResponseAction Run() override;
};

class BraveWalletGetWalletSeedFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveWallet.getWalletSeed", UNKNOWN)

  static std::string GetEthereumRemoteClientSeedFromRootSeed(
      const std::string& seed);
  static bool SealSeed(const std::string& seed, const std::string& key,
      const std::string& nonce, std::string* cipher_seed);
  static bool OpenSeed(const std::string& cipher_seed,
      const std::string& key, const std::string& nonce, std::string* seed);
  static void SaveToPrefs(Profile *, const std::string& cipher_seed,
      const std::string& nonce);
  static bool LoadFromPrefs(Profile *, std::string* cipher_seed,
      std::string* nonce);
  static std::string GetRandomNonce();
  static std::string GetRandomSeed();
  static const size_t kNonceByteLength;
  static const size_t kSeedByteLength;

 protected:
  ~BraveWalletGetWalletSeedFunction() override {}

  ResponseAction Run() override;
};

class BraveWalletGetProjectIDFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveWallet.getProjectID", UNKNOWN)

 protected:
  ~BraveWalletGetProjectIDFunction() override {}

  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_BRAVE_WALLET_API_H_
