/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BRAVE_WALLET_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_BRAVE_WALLET_API_H_

#include "extensions/browser/extension_function.h"

class Profile;

namespace extensions {
namespace api {

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
