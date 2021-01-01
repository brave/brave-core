/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_FTX_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_FTX_API_H_

#include <string>
#include <vector>

#include "brave/components/ftx/browser/ftx_service.h"
#include "extensions/browser/extension_function.h"

class Profile;

namespace extensions {
namespace api {

class FtxGetFuturesDataFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ftx.getFuturesData", UNKNOWN)

 protected:
  ~FtxGetFuturesDataFunction() override {}
  void OnFuturesData(const FTXFuturesData& data);

  ResponseAction Run() override;
};

class FtxGetChartDataFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ftx.getChartData", UNKNOWN)

 protected:
  ~FtxGetChartDataFunction() override {}
  void OnChartData(const FTXChartData& data);

  ResponseAction Run() override;
};

class FtxSetOauthHostFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ftx.setOauthHost", UNKNOWN)

 protected:
  ~FtxSetOauthHostFunction() override {}

  ResponseAction Run() override;
};

class FtxGetOauthHostFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ftx.getOauthHost", UNKNOWN)

 protected:
  ~FtxGetOauthHostFunction() override {}
  void OnOauthHost(const std::string& host);

  ResponseAction Run() override;
};

class FtxGetClientUrlFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ftx.getClientUrl", UNKNOWN)

 protected:
  ~FtxGetClientUrlFunction() override {}
  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_FTX_API_H_
