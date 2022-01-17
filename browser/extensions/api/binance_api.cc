/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/binance_api.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/environment.h"

#include "brave/common/extensions/api/binance.h"
#include "brave/common/pref_names.h"
#include "brave/browser/binance/binance_service_factory.h"
#include "brave/components/binance/browser/binance_service.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/constants.h"

namespace {

BinanceService* GetBinanceService(content::BrowserContext* context) {
  return BinanceServiceFactory::GetInstance()
      ->GetForProfile(Profile::FromBrowserContext(context));
}

bool IsBinanceAPIAvailable(content::BrowserContext* context) {
  Profile* profile = Profile::FromBrowserContext(context);
  return !profile->IsIncognitoProfile() && !profile->IsGuestSession();
}

}  // namespace

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction
BinanceGetUserTLDFunction::Run() {
  if (!IsBinanceAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  auto* service = GetBinanceService(browser_context());
  const std::string user_tld = service->GetBinanceTLD();

  return RespondNow(OneArgument(base::Value(user_tld)));
}

ExtensionFunction::ResponseAction
BinanceGetClientUrlFunction::Run() {
  if (!IsBinanceAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  auto* service = GetBinanceService(browser_context());
  const std::string client_url = service->GetOAuthClientUrl();

  return RespondNow(OneArgument(base::Value(client_url)));
}

ExtensionFunction::ResponseAction
BinanceGetAccessTokenFunction::Run() {
  if (!IsBinanceAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  auto* service = GetBinanceService(browser_context());
  bool token_request = service->GetAccessToken(base::BindOnce(
      &BinanceGetAccessTokenFunction::OnCodeResult, this));

  if (!token_request) {
    return RespondNow(
        Error("Could not make request for access tokens"));
  }

  return RespondLater();
}

void BinanceGetAccessTokenFunction::OnCodeResult(bool success) {
  Respond(OneArgument(base::Value(success)));
}

ExtensionFunction::ResponseAction
BinanceGetAccountBalancesFunction::Run() {
  if (!IsBinanceAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  auto* service = GetBinanceService(browser_context());
  bool balance_success = service->GetAccountBalances(
      base::BindOnce(
          &BinanceGetAccountBalancesFunction::OnGetAccountBalances,
          this));

  if (!balance_success) {
    return RespondNow(Error("Could not send request to get balance"));
  }

  return RespondLater();
}

void BinanceGetAccountBalancesFunction::OnGetAccountBalances(
    const BinanceAccountBalances& balances,
    bool success) {
  base::DictionaryValue result;

  for (const auto& balance : balances) {
    base::Value info(base::Value::Type::DICTIONARY);
    info.SetKey("balance", base::Value(balance.second[0]));
    info.SetKey("btcValue", base::Value(balance.second[1]));
    info.SetKey("fiatValue", base::Value(balance.second[2]));

    result.SetPath(balance.first, std::move(info));
  }

  Respond(TwoArguments(std::move(result), base::Value(success)));
}

ExtensionFunction::ResponseAction
BinanceGetConvertQuoteFunction::Run() {
  if (!IsBinanceAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  std::unique_ptr<binance::GetConvertQuote::Params> params(
      binance::GetConvertQuote::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto* service = GetBinanceService(browser_context());
  bool token_request = service->GetConvertQuote(
      params->from, params->to, params->amount,
      base::BindOnce(
          &BinanceGetConvertQuoteFunction::OnQuoteResult, this));

  if (!token_request) {
    return RespondNow(
        Error("Could not make request for access tokens"));
  }

  return RespondLater();
}

void BinanceGetConvertQuoteFunction::OnQuoteResult(
    const std::string& quote_id, const std::string& quote_price,
    const std::string& total_fee, const std::string& total_amount) {
  base::Value quote(base::Value::Type::DICTIONARY);
  quote.SetStringKey("id", quote_id);
  quote.SetStringKey("price", quote_price);
  quote.SetStringKey("fee", total_fee);
  quote.SetStringKey("amount", total_amount);
  Respond(OneArgument(std::move(quote)));
}

ExtensionFunction::ResponseAction
BinanceIsSupportedRegionFunction::Run() {
  if (!IsBinanceAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  auto* service = GetBinanceService(browser_context());
  bool is_supported = service->IsSupportedRegion();

  return RespondNow(OneArgument(base::Value(is_supported)));
}

ExtensionFunction::ResponseAction
BinanceGetDepositInfoFunction::Run() {
  if (!IsBinanceAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  std::unique_ptr<binance::GetDepositInfo::Params> params(
      binance::GetDepositInfo::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto* service = GetBinanceService(browser_context());
  bool info_request = service->GetDepositInfo(params->symbol,
      params->ticker_network,
      base::BindOnce(
          &BinanceGetDepositInfoFunction::OnGetDepositInfo, this));

  if (!info_request) {
    return RespondNow(
        Error("Could not make request for deposit information."));
  }

  return RespondLater();
}

void BinanceGetDepositInfoFunction::OnGetDepositInfo(
    const std::string& deposit_address,
    const std::string& deposit_tag,
    bool success) {
  Respond(TwoArguments(base::Value(deposit_address), base::Value(deposit_tag)));
}

ExtensionFunction::ResponseAction
BinanceConfirmConvertFunction::Run() {
  if (!IsBinanceAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  std::unique_ptr<binance::ConfirmConvert::Params> params(
      binance::ConfirmConvert::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto* service = GetBinanceService(browser_context());
  bool confirm_request = service->ConfirmConvert(params->quote_id,
      base::BindOnce(
          &BinanceConfirmConvertFunction::OnConfirmConvert, this));

  if (!confirm_request) {
    return RespondNow(
        Error("Could not confirm conversion"));
  }

  return RespondLater();
}

void BinanceConfirmConvertFunction::OnConfirmConvert(
    bool success, const std::string& message) {
  Respond(TwoArguments(base::Value(success), base::Value(message)));
}

ExtensionFunction::ResponseAction
BinanceGetConvertAssetsFunction::Run() {
  if (!IsBinanceAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  auto* service = GetBinanceService(browser_context());
  bool asset_request = service->GetConvertAssets(base::BindOnce(
          &BinanceGetConvertAssetsFunction::OnGetConvertAssets, this));

  if (!asset_request) {
    return RespondNow(
        Error("Could not retrieve supported convert assets"));
  }

  return RespondLater();
}

void BinanceGetConvertAssetsFunction::OnGetConvertAssets(
    const BinanceConvertAsserts& assets) {
  base::Value result(base::Value::Type::DICTIONARY);

  for (const auto& asset : assets) {
    base::ListValue sub_selectors;
    for (const auto& sub : asset.second) {
      auto sub_selector = std::make_unique<base::Value>(
          base::Value::Type::DICTIONARY);
      for (const auto& att : sub) {
        sub_selector->SetStringKey(att.first, att.second);
      }
      sub_selectors.Append(std::move(sub_selector));
    }
    result.SetKey(asset.first, std::move(sub_selectors));
  }

  Respond(OneArgument(std::move(result)));
}

ExtensionFunction::ResponseAction
BinanceRevokeTokenFunction::Run() {
  if (!IsBinanceAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  auto* service = GetBinanceService(browser_context());
  bool request = service->RevokeToken(base::BindOnce(
          &BinanceRevokeTokenFunction::OnRevokeToken, this));

  if (!request) {
    return RespondNow(
        Error("Could not revoke token"));
  }

  return RespondLater();
}

void BinanceRevokeTokenFunction::OnRevokeToken(bool success) {
  Respond(OneArgument(base::Value(success)));
}

ExtensionFunction::ResponseAction
BinanceGetCoinNetworksFunction::Run() {
  if (!IsBinanceAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  auto* service = GetBinanceService(browser_context());
  bool balance_success = service->GetCoinNetworks(
      base::BindOnce(
          &BinanceGetCoinNetworksFunction::OnGetCoinNetworks,
          this));

  if (!balance_success) {
    return RespondNow(Error("Could not send request to get coin networks"));
  }

  return RespondLater();
}

void BinanceGetCoinNetworksFunction::OnGetCoinNetworks(
    const BinanceCoinNetworks& networks) {
  base::Value coin_networks(base::Value::Type::DICTIONARY);

  for (const auto& network : networks) {
    coin_networks.SetStringKey(network.first, network.second);
  }

  Respond(OneArgument(std::move(coin_networks)));
}

ExtensionFunction::ResponseAction BinanceGetLocaleForURLFunction::Run() {
  if (!IsBinanceAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  auto* service = GetBinanceService(browser_context());
  const std::string locale = service->GetLocaleForURL();

  return RespondNow(OneArgument(base::Value(locale)));
}

}  // namespace api
}  // namespace extensions
