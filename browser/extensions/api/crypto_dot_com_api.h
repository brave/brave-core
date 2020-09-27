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
#include "brave/components/crypto_dot_com/browser/crypto_dot_com_service.h"

class Profile;

namespace extensions {
namespace api {

class CryptoDotComGetTickerInfoFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cryptoDotCom.getTickerInfo", UNKNOWN)

 protected:
  ~CryptoDotComGetTickerInfoFunction() override {}
  void OnInfoResult(const CryptoDotComTickerInfo& info);

  ResponseAction Run() override;
};

class CryptoDotComGetChartDataFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cryptoDotCom.getChartData", UNKNOWN)

 protected:
  ~CryptoDotComGetChartDataFunction() override {}
  void OnChartDataResult(
      const CryptoDotComChartData& data);

  ResponseAction Run() override;
};

class CryptoDotComGetSupportedPairsFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cryptoDotCom.getSupportedPairs", UNKNOWN)

 protected:
  ~CryptoDotComGetSupportedPairsFunction() override {}
  void OnSupportedPairsResult(
      const CryptoDotComSupportedPairs& pairs);

  ResponseAction Run() override;
};

class CryptoDotComGetAssetRankingsFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cryptoDotCom.getAssetRankings", UNKNOWN)

 protected:
  ~CryptoDotComGetAssetRankingsFunction() override {}
  void OnAssetRankingsResult(
      const CryptoDotComAssetRankings& rankings);

  ResponseAction Run() override;
};

class CryptoDotComIsSupportedFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cryptoDotCom.isSupported", UNKNOWN)

 protected:
  ~CryptoDotComIsSupportedFunction() override {}
  ResponseAction Run() override;
};

class CryptoDotComOnBuyCryptoFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cryptoDotCom.onBuyCrypto", UNKNOWN)

 protected:
  ~CryptoDotComOnBuyCryptoFunction() override {}
  ResponseAction Run() override;
};

class CryptoDotComOnInteractionFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cryptoDotCom.onInteraction", UNKNOWN)

 protected:
  ~CryptoDotComOnInteractionFunction() override {}
  ResponseAction Run() override;
};

class CryptoDotComGetInteractionsFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cryptoDotCom.getInteractions", UNKNOWN)

 protected:
  ~CryptoDotComGetInteractionsFunction() override {}
  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_CRYPTO_DOT_COM_API_H_
