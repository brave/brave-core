/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_GEMINI_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_GEMINI_API_H_

#include <map>
#include <string>
#include <vector>

#include "extensions/browser/extension_function.h"

class Profile;

namespace extensions {
namespace api {

class GeminiGetClientUrlFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("gemini.getClientUrl", UNKNOWN)

 protected:
  ~GeminiGetClientUrlFunction() override {}
  ResponseAction Run() override;
};

class GeminiGetAccessTokenFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("gemini.getAccessToken", UNKNOWN)

 protected:
  ~GeminiGetAccessTokenFunction() override {}
  void OnCodeResult(bool success);

  ResponseAction Run() override;
};

class GeminiGetTickerPriceFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("gemini.getTickerPrice", UNKNOWN)

 protected:
  ~GeminiGetTickerPriceFunction() override {}
  void OnPriceResult(const std::string& price);

  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_GEMINI_API_H_
