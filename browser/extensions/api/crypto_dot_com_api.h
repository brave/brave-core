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

class CryptoDotComGetAccountBalancesFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cryptoDotCom.getAccountBalances", UNKNOWN)

 protected:
  ~CryptoDotComGetAccountBalancesFunction() override {}
  ResponseAction Run() override;
  void OnGetAccountBalancesResult(base::Value balances);
};

class CryptoDotComGetClientUrlFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cryptoDotCom.getClientUrl", UNKNOWN)

 protected:
  ~CryptoDotComGetClientUrlFunction() override {}
  ResponseAction Run() override;
};

class CryptoDotComIsConnectedFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cryptoDotCom.isConnected", UNKNOWN)

 protected:
  ~CryptoDotComIsConnectedFunction() override {}
  ResponseAction Run() override;
  void OnIsConnectedResult(bool connected);
};

class CryptoDotComDisconnectFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cryptoDotCom.disconnect", UNKNOWN)

 protected:
  ~CryptoDotComDisconnectFunction() override {}
  ResponseAction Run() override;
};

class CryptoDotComIsLoggedInFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cryptoDotCom.isLoggedIn", UNKNOWN)

 protected:
  ~CryptoDotComIsLoggedInFunction() override {}
  ResponseAction Run() override;
};

class CryptoDotComGetNewsEventsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cryptoDotCom.getNewsEvents", UNKNOWN)

 protected:
  ~CryptoDotComGetNewsEventsFunction() override {}
  ResponseAction Run() override;
  void OnGetNewsEventsResult(base::Value events);
};

class CryptoDotComGetDepositAddressFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cryptoDotCom.getDepositAddress", UNKNOWN)

 protected:
  ~CryptoDotComGetDepositAddressFunction() override {}
  ResponseAction Run() override;
  void OnGetDepositAddressResult(base::Value address);
};

class CryptoDotComCreateMarketOrderFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cryptoDotCom.createMarketOrder", UNKNOWN)

 protected:
  ~CryptoDotComCreateMarketOrderFunction() override {}
  ResponseAction Run() override;
  void OnCreateMarketOrderResult(base::Value result);
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_CRYPTO_DOT_COM_API_H_
