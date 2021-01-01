/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/ftx_api.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/browser/ftx/ftx_service_factory.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/common/extensions/api/ftx.h"
#include "brave/components/ftx/browser/ftx_service.h"
#include "brave/components/ftx/common/pref_names.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/constants.h"

namespace {

FTXService* GetFTXService(content::BrowserContext* context) {
  return FTXServiceFactory::GetInstance()->GetForProfile(
      Profile::FromBrowserContext(context));
}

bool IsFTXAPIAvailable(content::BrowserContext* context) {
  return brave::IsRegularProfile(context);
}

}  // namespace

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction FtxGetFuturesDataFunction::Run() {
  if (!IsFTXAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  auto* service = GetFTXService(browser_context());
  bool data_request = service->GetFuturesData(
      base::BindOnce(&FtxGetFuturesDataFunction::OnFuturesData, this));

  if (!data_request) {
    return RespondNow(Error("Could not make request for futures data"));
  }

  return RespondLater();
}

void FtxGetFuturesDataFunction::OnFuturesData(const FTXFuturesData& data) {
  base::ListValue result;

  for (const auto& data_point : data) {
    auto point = std::make_unique<base::Value>(base::Value::Type::DICTIONARY);
    for (const auto& att : data_point) {
      point->SetStringKey(att.first, att.second);
    }
    result.Append(std::move(point));
  }

  Respond(OneArgument(std::move(result)));
}

ExtensionFunction::ResponseAction FtxGetChartDataFunction::Run() {
  if (!IsFTXAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  std::unique_ptr<ftx::GetChartData::Params> params(
      ftx::GetChartData::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto* service = GetFTXService(browser_context());
  bool data_request = service->GetChartData(
      params->symbol, params->start, params->end,
      base::BindOnce(&FtxGetChartDataFunction::OnChartData, this));

  if (!data_request) {
    return RespondNow(Error("Could not make request for chart data"));
  }

  return RespondLater();
}

void FtxGetChartDataFunction::OnChartData(const FTXChartData& data) {
  base::ListValue result;

  for (const auto& data_point : data) {
    auto point = std::make_unique<base::Value>(base::Value::Type::DICTIONARY);
    for (const auto& att : data_point) {
      point->SetStringKey(att.first, att.second);
    }
    result.Append(std::move(point));
  }

  Respond(OneArgument(std::move(result)));
}

ExtensionFunction::ResponseAction FtxSetOauthHostFunction::Run() {
  if (!IsFTXAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  std::unique_ptr<ftx::SetOauthHost::Params> params(
      ftx::SetOauthHost::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  if (params->host != "ftx.us" && params->host != "ftx.com") {
    return RespondNow(NoArguments());
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  profile->GetPrefs()->SetString(kFTXOauthHost, params->host);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction FtxGetOauthHostFunction::Run() {
  if (!IsFTXAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  const std::string host = profile->GetPrefs()->GetString(kFTXOauthHost);

  return RespondNow(OneArgument(base::Value(host)));
}

ExtensionFunction::ResponseAction FtxGetClientUrlFunction::Run() {
  if (!IsFTXAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  auto* service = GetFTXService(browser_context());
  const std::string client_url = service->GetOAuthClientUrl();

  return RespondNow(OneArgument(base::Value(client_url)));
}

ExtensionFunction::ResponseAction FtxGetAccessTokenFunction::Run() {
  if (!IsFTXAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  auto* service = GetFTXService(browser_context());
  bool token_request = service->GetAccessToken(
      base::BindOnce(&FtxGetAccessTokenFunction::OnCodeResult, this));

  if (!token_request) {
    return RespondNow(Error("Could not make request for access token"));
  }

  return RespondLater();
}

void FtxGetAccessTokenFunction::OnCodeResult(bool success) {
  Respond(OneArgument(base::Value(success)));
}

}  // namespace api
}  // namespace extensions
