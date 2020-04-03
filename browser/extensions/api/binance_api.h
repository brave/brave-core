/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BINANCE_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_BINANCE_API_H_

#include <map>
#include <string>
#include <vector>

#include "extensions/browser/extension_function.h"

class Profile;

namespace extensions {
namespace api {

class BinanceGetUserTLDFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("binance.getUserTLD", UNKNOWN)

 protected:
  ~BinanceGetUserTLDFunction() override {}
  ResponseAction Run() override;
};

class BinanceIsSupportedRegionFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("binance.isSupportedRegion", UNKNOWN)

 protected:
  ~BinanceIsSupportedRegionFunction() override {}
  ResponseAction Run() override;
};

class BinanceGetClientUrlFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("binance.getClientUrl", UNKNOWN)

 protected:
  ~BinanceGetClientUrlFunction() override {}
  ResponseAction Run() override;
};

class BinanceGetAccessTokenFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("binance.getAccessToken", UNKNOWN)

 protected:
  ~BinanceGetAccessTokenFunction() override {}
  void OnCodeResult(bool success);

  ResponseAction Run() override;
};

class BinanceGetAccountBalancesFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("binance.getAccountBalances", UNKNOWN)

 protected:
  ~BinanceGetAccountBalancesFunction() override {}
  void OnGetAccountBalances(const std::map<std::string, std::string>& balances,
                           bool success);

  ResponseAction Run() override;
};

class BinanceGetConvertQuoteFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("binance.getConvertQuote", UNKNOWN)

 protected:
  ~BinanceGetConvertQuoteFunction() override {}
  void OnQuoteResult(const std::string& quote_id,
                     const std::string& quote_price,
                     const std::string& total_fee,
                     const std::string& total_amount);

  ResponseAction Run() override;
};

class BinanceGetTickerPriceFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("binance.getTickerPrice", UNKNOWN)

 protected:
  ~BinanceGetTickerPriceFunction() override {}
  void OnGetTickerPrice(const std::string& symbol_pair_price);

  ResponseAction Run() override;
};

class BinanceGetTickerVolumeFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("binance.getTickerVolume", UNKNOWN)

 protected:
  ~BinanceGetTickerVolumeFunction() override {}
  void OnGetTickerVolume(const std::string& symbol_pair_volume);

  ResponseAction Run() override;
};

class BinanceGetDepositInfoFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("binance.getDepositInfo", UNKNOWN)

 protected:
  ~BinanceGetDepositInfoFunction() override {}
  void OnGetDepositInfo(const std::string& deposit_address,
                        const std::string& deposit_url,
                        bool success);

  ResponseAction Run() override;
};

class BinanceConfirmConvertFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("binance.confirmConvert", UNKNOWN)

 protected:
  ~BinanceConfirmConvertFunction() override {}
  void OnConfirmConvert(bool success);

  ResponseAction Run() override;
};

class BinanceGetConvertAssetsFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("binance.getConvertAssets", UNKNOWN)

 protected:
  ~BinanceGetConvertAssetsFunction() override {}
  void OnGetConvertAssets(
      const std::map<std::string, std::vector<std::string>>& assets);

  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_BINANCE_API_H_
