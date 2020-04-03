/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/binance_api.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/environment.h"
#include "brave/browser/profiles/profile_util.h"

#include "brave/common/extensions/api/binance.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/common/pref_names.h"
#include "brave/browser/binance/binance_service_factory.h"
#include "brave/components/binance/browser/binance_service.h"
#include "brave/components/binance/browser/static_values.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "components/country_codes/country_codes.h"
#include "extensions/browser/extension_util.h"

namespace {

BinanceService* GetBinanceService(content::BrowserContext* context) {
  return BinanceServiceFactory::GetInstance()
      ->GetForProfile(Profile::FromBrowserContext(context));
}

}  // namespace

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction
BinanceGetUserTLDFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (brave::IsTorProfile(profile)) {
    return RespondNow(Error("Not available in Tor profile"));
  }

  auto* service = GetBinanceService(browser_context());
  const std::string userTLD = service->GetBinanceTLD();

  return RespondNow(OneArgument(
      std::make_unique<base::Value>(userTLD)));
}

ExtensionFunction::ResponseAction
BinanceGetClientUrlFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (brave::IsTorProfile(profile)) {
    return RespondNow(Error("Not available in Tor profile"));
  }

  auto* service = GetBinanceService(browser_context());
  const std::string client_url = service->GetOAuthClientUrl();

  return RespondNow(OneArgument(
      std::make_unique<base::Value>(client_url)));
}

ExtensionFunction::ResponseAction
BinanceGetAccessTokenFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (brave::IsTorProfile(profile)) {
    return RespondNow(Error("Not available in Tor profile"));
  }

  std::unique_ptr<binance::GetAccessToken::Params> params(
      binance::GetAccessToken::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto* service = GetBinanceService(browser_context());
  bool token_request = service->GetAccessToken(params->code,
      base::BindOnce(
          &BinanceGetAccessTokenFunction::OnCodeResult, this));

  if (!token_request) {
    return RespondNow(
        Error("Could not make request for access tokens"));
  }

  return RespondLater();
}

void BinanceGetAccessTokenFunction::OnCodeResult(bool success) {
  Respond(OneArgument(std::make_unique<base::Value>(success)));
}

ExtensionFunction::ResponseAction
BinanceGetAccountBalancesFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (brave::IsTorProfile(profile)) {
    return RespondNow(Error("Not available in Tor profile"));
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
    const std::map<std::string, std::string>& balances, bool success) {
  auto balance_dict = std::make_unique<base::Value>(
      base::Value::Type::DICTIONARY);

  for (const auto& balance : balances) {
    balance_dict->SetStringKey(balance.first, balance.second);
  }

  Respond(TwoArguments(std::move(balance_dict),
                       std::make_unique<base::Value>(success)));
}

ExtensionFunction::ResponseAction
BinanceGetConvertQuoteFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (brave::IsTorProfile(profile)) {
    return RespondNow(Error("Not available in Tor profile"));
  }

  std::unique_ptr<binance::GetConvertQuote::Params> params(
      binance::GetConvertQuote::Params::Create(*args_));
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
  auto quote = std::make_unique<base::Value>(base::Value::Type::DICTIONARY);
  quote->SetStringKey("id", quote_id);
  quote->SetStringKey("price", quote_price);
  quote->SetStringKey("fee", total_fee);
  quote->SetStringKey("amount", total_amount);
  Respond(OneArgument(std::move(quote)));
}

ExtensionFunction::ResponseAction
BinanceGetTickerPriceFunction::Run() {
  std::unique_ptr<binance::GetTickerPrice::Params> params(
      binance::GetTickerPrice::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (brave::IsTorProfile(profile)) {
    return RespondNow(Error("Not available in Tor profile"));
  }

  auto* service = GetBinanceService(browser_context());
  bool value_request = service->GetTickerPrice(params->symbol_pair,
      base::BindOnce(
          &BinanceGetTickerPriceFunction::OnGetTickerPrice, this));

  if (!value_request) {
    return RespondNow(
        Error("Could not make request for BTC price"));
  }

  return RespondLater();
}

void BinanceGetTickerPriceFunction::OnGetTickerPrice(
    const std::string& symbol_pair_price) {
  Respond(OneArgument(std::make_unique<base::Value>(symbol_pair_price)));
}

ExtensionFunction::ResponseAction
BinanceGetTickerVolumeFunction::Run() {
  std::unique_ptr<binance::GetTickerVolume::Params> params(
      binance::GetTickerVolume::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (brave::IsTorProfile(profile)) {
    return RespondNow(Error("Not available in Tor profile"));
  }

  auto* service = GetBinanceService(browser_context());
  bool value_request = service->GetTickerVolume(params->symbol_pair,
      base::BindOnce(
          &BinanceGetTickerVolumeFunction::OnGetTickerVolume, this));

  if (!value_request) {
    return RespondNow(
        Error("Could not make request for Volume"));
  }

  return RespondLater();
}

void BinanceGetTickerVolumeFunction::OnGetTickerVolume(
    const std::string& symbol_pair_volume) {
  Respond(OneArgument(std::make_unique<base::Value>(symbol_pair_volume)));
}

ExtensionFunction::ResponseAction
BinanceIsSupportedRegionFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (brave::IsTorProfile(profile)) {
    return RespondNow(Error("Not available in Tor profile"));
  }

  bool is_blacklisted = false;
  const int32_t user_country_id =
      country_codes::GetCountryIDFromPrefs(profile->GetPrefs());

  for (const auto& country : ::binance::kBinanceBlacklistRegions) {
    const int id = country_codes::CountryCharsToCountryID(
        country.at(0), country.at(1));

    if (id == user_country_id) {
      is_blacklisted = true;
      break;
    }
  }

  return RespondNow(OneArgument(
      std::make_unique<base::Value>(!is_blacklisted)));
}

ExtensionFunction::ResponseAction
BinanceGetDepositInfoFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (brave::IsTorProfile(profile)) {
    return RespondNow(Error("Not available in Tor profile"));
  }

  std::unique_ptr<binance::GetDepositInfo::Params> params(
      binance::GetDepositInfo::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto* service = GetBinanceService(browser_context());
  bool info_request = service->GetDepositInfo(params->symbol,
      base::BindOnce(
          &BinanceGetDepositInfoFunction::OnGetDepositInfo, this));

  if (!info_request) {
    return RespondNow(
        Error("Could not make request for Volume"));
  }

  return RespondLater();
}

void BinanceGetDepositInfoFunction::OnGetDepositInfo(
    const std::string& deposit_address,
    const std::string& deposit_url,
    bool success) {
  Respond(TwoArguments(
      std::make_unique<base::Value>(deposit_address),
      std::make_unique<base::Value>(deposit_url)));
}

ExtensionFunction::ResponseAction
BinanceConfirmConvertFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (brave::IsTorProfile(profile)) {
    return RespondNow(Error("Not available in Tor profile"));
  }

  std::unique_ptr<binance::ConfirmConvert::Params> params(
      binance::ConfirmConvert::Params::Create(*args_));
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

void BinanceConfirmConvertFunction::OnConfirmConvert(bool success) {
  Respond(OneArgument(std::make_unique<base::Value>(success)));
}

ExtensionFunction::ResponseAction
BinanceGetConvertAssetsFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (brave::IsTorProfile(profile)) {
    return RespondNow(Error("Not available in Tor profile"));
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
    const std::map<std::string, std::vector<std::string>>& assets) {
  std::unique_ptr<base::DictionaryValue> asset_dict(
      new base::DictionaryValue());

  for (const auto& asset : assets) {
    auto supported = std::make_unique<base::ListValue>();
    if (!asset.second.empty()) {
      for (auto const& ticker : asset.second) {
        supported->Append(ticker);
      }
    }
    asset_dict->SetList(asset.first, std::move(supported));
  }

  Respond(OneArgument(std::move(asset_dict)));
}

}  // namespace api
}  // namespace extensions
