/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_CONTRIBUTE_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_CONTRIBUTE_API_H_

#include <map>
#include <string>
#include <vector>

#include "extensions/browser/extension_function.h"

class Profile;

namespace extensions {
namespace api {

class ContributeGetUserTLDFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("contribute.getUserTLD", UNKNOWN)

 protected:
  ~ContributeGetUserTLDFunction() override {}
  ResponseAction Run() override;
};

class ContributeIsSupportedRegionFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("contribute.isSupportedRegion", UNKNOWN)

 protected:
  ~ContributeIsSupportedRegionFunction() override {}
  ResponseAction Run() override;
};

class ContributeGetClientUrlFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("contribute.getClientUrl", UNKNOWN)

 protected:
  ~ContributeGetClientUrlFunction() override {}
  ResponseAction Run() override;
};

class ContributeGetAccessTokenFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("contribute.getAccessToken", UNKNOWN)

 protected:
  ~ContributeGetAccessTokenFunction() override {}
  void OnCodeResult(bool success);

  ResponseAction Run() override;
};

class ContributeGetAccountBalancesFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("contribute.getAccountBalances", UNKNOWN)

 protected:
  ~ContributeGetAccountBalancesFunction() override {}
  void OnGetAccountBalances(const std::map<std::string, std::string>& balances,
                            bool success);

  ResponseAction Run() override;
};

class ContributeGetConvertQuoteFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("contribute.getConvertQuote", UNKNOWN)

 protected:
  ~ContributeGetConvertQuoteFunction() override {}
  void OnQuoteResult(const std::string& quote_id,
                     const std::string& quote_price,
                     const std::string& total_fee,
                     const std::string& total_amount);

  ResponseAction Run() override;
};

class ContributeGetTickerPriceFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("contribute.getTickerPrice", UNKNOWN)

 protected:
  ~ContributeGetTickerPriceFunction() override {}
  void OnGetTickerPrice(const std::string& symbol_pair_price);

  ResponseAction Run() override;
};

class ContributeGetTickerVolumeFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("contribute.getTickerVolume", UNKNOWN)

 protected:
  ~ContributeGetTickerVolumeFunction() override {}
  void OnGetTickerVolume(const std::string& symbol_pair_volume);

  ResponseAction Run() override;
};

class ContributeGetDepositInfoFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("contribute.getDepositInfo", UNKNOWN)

 protected:
  ~ContributeGetDepositInfoFunction() override {}
  void OnGetDepositInfo(const std::string& deposit_address,
                        const std::string& deposit_url,
                        bool success);

  ResponseAction Run() override;
};

class ContributeConfirmConvertFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("contribute.confirmConvert", UNKNOWN)

 protected:
  ~ContributeConfirmConvertFunction() override {}
  void OnConfirmConvert(bool success, const std::string& message);

  ResponseAction Run() override;
};

class ContributeGetConvertAssetsFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("contribute.getConvertAssets", UNKNOWN)

 protected:
  ~ContributeGetConvertAssetsFunction() override {}
  void OnGetConvertAssets(
      const std::map<std::string, std::vector<std::string>>& assets);

  ResponseAction Run() override;
};

class ContributeRevokeTokenFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("contribute.revokeToken", UNKNOWN)

 protected:
  ~ContributeRevokeTokenFunction() override {}
  void OnRevokeToken(bool success);

  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_CONTRIBUTE_API_H_
