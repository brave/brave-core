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

class BraveWalletReadyFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveWallet.ready", UNKNOWN)

 protected:
  ~BraveWalletReadyFunction() override {}
  ResponseAction Run() override;
};

class BraveWalletNotifyWalletUnlockFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveWallet.notifyWalletUnlock", UNKNOWN)

 protected:
  ~BraveWalletNotifyWalletUnlockFunction() override {}
  ResponseAction Run() override;
};

class BraveWalletLoadUIFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveWallet.loadUI", UNKNOWN)
  void OnLoaded();

 protected:
  ~BraveWalletLoadUIFunction() override {}
  ResponseAction Run() override;
};

class BraveWalletShouldPromptForSetupFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveWallet.shouldPromptForSetup", UNKNOWN)

 protected:
  ~BraveWalletShouldPromptForSetupFunction() override {}
  ResponseAction Run() override;
};

class BraveWalletGetWalletSeedFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveWallet.getWalletSeed", UNKNOWN)

 protected:
  ~BraveWalletGetWalletSeedFunction() override {}
  ResponseAction Run() override;
};

class BraveWalletGetBitGoSeedFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveWallet.getBitGoSeed", UNKNOWN)

 protected:
  ~BraveWalletGetBitGoSeedFunction() override {}
  ResponseAction Run() override;
};

class BraveWalletGetProjectIDFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveWallet.getProjectID", UNKNOWN)

 protected:
  ~BraveWalletGetProjectIDFunction() override {}
  ResponseAction Run() override;
};

class BraveWalletGetBraveKeyFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveWallet.getBraveKey", UNKNOWN)

 protected:
  ~BraveWalletGetBraveKeyFunction() override {}
  ResponseAction Run() override;
};

class BraveWalletResetWalletFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveWallet.resetWallet", UNKNOWN)

 protected:
  ~BraveWalletResetWalletFunction() override {}
  ResponseAction Run() override;
};

class BraveWalletGetWeb3ProviderFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveWallet.getWeb3Provider", UNKNOWN)

 protected:
  ~BraveWalletGetWeb3ProviderFunction() override {}
  ResponseAction Run() override;
};

class BraveWalletGetWeb3ProviderListFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveWallet.getWeb3ProviderList", UNKNOWN)

 protected:
  ~BraveWalletGetWeb3ProviderListFunction() override {}
  ResponseAction Run() override;
};

class BraveWalletIsNativeWalletEnabledFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveWallet.isNativeWalletEnabled", UNKNOWN)

 protected:
  ~BraveWalletIsNativeWalletEnabledFunction() override {}
  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_BRAVE_WALLET_API_H_
