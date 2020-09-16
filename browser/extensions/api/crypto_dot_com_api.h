/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_CRYPTO_DOT_COM_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_CRYPTO_DOT_COM_API_H_

#include <map>
#include <string>
#include <vector>

#include "extensions/browser/extension_function.h"

class Profile;

namespace extensions {
namespace api {

class CryptoDotComGetTickerInfoFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cryptoDotCom.getTickerInfo", UNKNOWN)

 protected:
  ~CryptoDotComGetTickerInfoFunction() override {}
  void OnInfoResult(const std::map<std::string, std::string>& info);

  ResponseAction Run() override;
};

class CryptoDotComGetChartDataFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cryptoDotCom.getChartData", UNKNOWN)

 protected:
  ~CryptoDotComGetChartDataFunction() override {}
  void OnChartDataResult(
      const std::vector<std::map<std::string, std::string>>& data);

  ResponseAction Run() override;
};

class CryptoDotComGetSupportedPairsFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cryptoDotCom.getSupportedPairs", UNKNOWN)

 protected:
  ~CryptoDotComGetSupportedPairsFunction() override {}
  void OnSupportedPairsResult(
      const std::vector<std::map<std::string, std::string>>& pairs);

  ResponseAction Run() override;
};

class CryptoDotComGetAssetRankingsFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cryptoDotCom.getAssetRankings", UNKNOWN)

 protected:
  ~CryptoDotComGetAssetRankingsFunction() override {}
  void OnAssetRankingsResult(
      const std::map<std::string,
      std::vector<std::map<std::string, std::string>>>& rankings);

  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_CRYPTO_DOT_COM_API_H_
