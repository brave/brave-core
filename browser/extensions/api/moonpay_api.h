/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_MOONPAY_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_MOONPAY_API_H_

#include <map>
#include <string>

#include "extensions/browser/extension_function.h"

class Profile;

namespace extensions {
namespace api {

class MoonpayIsBitcoinDotComSupportedFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("moonpay.isBitcoinDotComSupported", UNKNOWN)

 protected:
  ~MoonpayIsBitcoinDotComSupportedFunction() override {}
  ResponseAction Run() override;
};

class MoonpayOnBuyBitcoinDotComCryptoFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("moonpay.onBuyBitcoinDotComCrypto", UNKNOWN)

 protected:
  ~MoonpayOnBuyBitcoinDotComCryptoFunction() override {}
  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_MOONPAY_API_H_
