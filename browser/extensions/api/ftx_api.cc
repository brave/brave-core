// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/extensions/api/ftx_api.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/browser/ftx/ftx_service_factory.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/common/extensions/api/ftx.h"
#include "brave/components/ftx/browser/ftx_service.h"
#include "brave/components/ftx/browser/regions.h"
#include "brave/components/ftx/common/pref_names.h"
#include "brave/components/ntp_widget_utils/browser/ntp_widget_utils_region.h"
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

}  // namespace

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction FtxGetFuturesDataFunction::Run() {
  auto* service = GetFTXService(browser_context());
  if (!service) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  bool data_request = service->GetFuturesData(
      base::BindOnce(&FtxGetFuturesDataFunction::OnFuturesData, this));

  if (!data_request) {
    return RespondNow(Error("Could not make request for futures data"));
  }

  return RespondLater();
}

void FtxGetFuturesDataFunction::OnFuturesData(const FTXFuturesData& data) {
  base::ListValue result;

  for (const TokenPriceData& currency : data) {
    auto point = std::make_unique<base::Value>(base::Value::Type::DICTIONARY);
    point->SetStringKey("symbol", currency.symbol);
    point->SetDoubleKey("price", currency.price);
    point->SetDoubleKey("percentChangeDay", currency.percentChangeDay);
    point->SetDoubleKey("volumeDay", currency.volumeDay);
    result.Append(std::move(point));
  }

  Respond(OneArgument(std::move(result)));
}

ExtensionFunction::ResponseAction FtxGetChartDataFunction::Run() {
  auto* service = GetFTXService(browser_context());
  if (!service) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  std::unique_ptr<ftx::GetChartData::Params> params(
      ftx::GetChartData::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

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
      point->SetDoubleKey(att.first, att.second);
    }
    result.Append(std::move(point));
  }

  Respond(OneArgument(std::move(result)));
}

ExtensionFunction::ResponseAction FtxSetOauthHostFunction::Run() {
  auto* service = GetFTXService(browser_context());
  if (!service) {
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
  auto* service = GetFTXService(browser_context());
  if (!service) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  const std::string host = profile->GetPrefs()->GetString(kFTXOauthHost);

  return RespondNow(OneArgument(base::Value(host)));
}

ExtensionFunction::ResponseAction FtxGetClientUrlFunction::Run() {
  auto* service = GetFTXService(browser_context());
  if (!service) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  const std::string client_url = service->GetOAuthClientUrl();

  return RespondNow(OneArgument(base::Value(client_url)));
}

ExtensionFunction::ResponseAction FtxDisconnectFunction::Run() {
  auto* service = GetFTXService(browser_context());
  if (!service) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  service->ClearAuth();

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction FtxGetAccountBalancesFunction::Run() {
  auto* service = GetFTXService(browser_context());
  if (!service) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  bool balance_success = service->GetAccountBalances(base::BindOnce(
      &FtxGetAccountBalancesFunction::OnGetAccountBalances, this));

  if (!balance_success) {
    return RespondNow(Error("Could not send request to get balance"));
  }

  return RespondLater();
}

void FtxGetAccountBalancesFunction::OnGetAccountBalances(
    const FTXAccountBalances& balances,
    bool auth_invalid) {
  base::Value result(base::Value::Type::DICTIONARY);

  for (const auto& balance : balances) {
    result.SetDoubleKey(balance.first, balance.second);
  }

  Respond(TwoArguments(std::move(result), base::Value(auth_invalid)));
}

ExtensionFunction::ResponseAction FtxIsSupportedFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  bool is_supported = ntp_widget_utils::IsRegionSupported(
      profile->GetPrefs(), ::ftx::unsupported_regions, false);
  return RespondNow(OneArgument(base::Value(is_supported)));
}

ExtensionFunction::ResponseAction FtxGetConvertQuoteFunction::Run() {
  auto* service = GetFTXService(browser_context());
  if (!service) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  std::unique_ptr<ftx::GetConvertQuote::Params> params(
      ftx::GetConvertQuote::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  bool data_request = service->GetConvertQuote(
      params->from, params->to, params->amount,
      base::BindOnce(&FtxGetConvertQuoteFunction::OnConvertQuote, this));

  if (!data_request) {
    return RespondNow(Error("Could not make request for convert quote"));
  }

  return RespondLater();
}

void FtxGetConvertQuoteFunction::OnConvertQuote(const std::string& quote_id) {
  Respond(OneArgument(base::Value(quote_id)));
}

ExtensionFunction::ResponseAction FtxGetConvertQuoteInfoFunction::Run() {
  auto* service = GetFTXService(browser_context());
  if (!service) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  std::unique_ptr<ftx::GetConvertQuoteInfo::Params> params(
      ftx::GetConvertQuoteInfo::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  bool data_request = service->GetConvertQuoteInfo(
      params->quote_id,
      base::BindOnce(
          &FtxGetConvertQuoteInfoFunction::OnConvertQuoteInfo, this));

  if (!data_request) {
    return RespondNow(Error("Could not make request for convert quote info"));
  }

  return RespondLater();
}

void FtxGetConvertQuoteInfoFunction::OnConvertQuoteInfo(const std::string& cost,
                                                 const std::string& price,
                                                 const std::string& proceeds) {
  base::Value quote(base::Value::Type::DICTIONARY);
  quote.SetStringKey("cost", cost);
  quote.SetStringKey("price", price);
  quote.SetStringKey("proceeds", proceeds);
  Respond(OneArgument(std::move(quote)));
}

ExtensionFunction::ResponseAction FtxExecuteConvertQuoteFunction::Run() {
  auto* service = GetFTXService(browser_context());
  if (!service) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  std::unique_ptr<ftx::ExecuteConvertQuote::Params> params(
      ftx::ExecuteConvertQuote::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  bool data_request = service->ExecuteConvertQuote(
      params->quote_id,
      base::BindOnce(
          &FtxExecuteConvertQuoteFunction::OnExecuteConvertQuote, this));

  if (!data_request) {
    return RespondNow(Error("Could not make request to execute quote"));
  }

  return RespondLater();
}

void FtxExecuteConvertQuoteFunction::OnExecuteConvertQuote(bool success) {
  Respond(OneArgument(base::Value(success)));
}

}  // namespace api
}  // namespace extensions
