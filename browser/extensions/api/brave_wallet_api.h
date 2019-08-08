/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BRAVE_WALLET_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_BRAVE_WALLET_API_H_

#include "extensions/browser/extension_function.h"

namespace extensions {
namespace api {

class BraveWalletPromptToEnableWalletFunction :
    public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveWallet.promptToEnableWallet", UNKNOWN)

 protected:
  ~BraveWalletPromptToEnableWalletFunction() override {}

  ResponseAction Run() override;
};

class BraveWalletIsEnabledFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveWallet.isEnabled", UNKNOWN)

 protected:
  ~BraveWalletIsEnabledFunction() override {}

  ResponseAction Run() override;
};

class BraveWalletGetWalletSeedFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveWallet.getWalletSeed", UNKNOWN)

 protected:
  ~BraveWalletGetWalletSeedFunction() override {}

  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_BRAVE_WALLET_API_H_
