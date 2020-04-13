/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/contribute_api.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/environment.h"
#include "brave/browser/profiles/profile_util.h"

#include "brave/common/extensions/api/contribute.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/common/pref_names.h"
#include "brave/browser/contribute/contribute_service_factory.h"
#include "brave/components/contribute/browser/contribute_service.h"
#include "brave/components/contribute/browser/static_values.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "components/country_codes/country_codes.h"
#include "extensions/browser/extension_util.h"

namespace {

ContributeService* GetContributeService(content::BrowserContext* context) {
  return ContributeServiceFactory::GetInstance()
      ->GetForProfile(Profile::FromBrowserContext(context));
}

bool IsContributeAPIAvailable(content::BrowserContext* context) {
  Profile* profile = Profile::FromBrowserContext(context);
  return !brave::IsTorProfile(profile) &&
    !profile->IsIncognitoProfile() &&
    !profile->IsGuestSession();
}

}  // namespace

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction
ContributeGetUserTLDFunction::Run() {
  if (!IsContributeAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  auto* service = GetContributeService(browser_context());
  const std::string user_tld = service->GetContributeTLD();

  return RespondNow(OneArgument(
      std::make_unique<base::Value>(user_tld)));
}

ExtensionFunction::ResponseAction
ContributeGetClientUrlFunction::Run() {
  if (!IsContributeAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  auto* service = GetContributeService(browser_context());
  const std::string client_url = service->GetOAuthClientUrl();

  return RespondNow(OneArgument(
      std::make_unique<base::Value>(client_url)));
}

ExtensionFunction::ResponseAction
ContributeGetAccessTokenFunction::Run() {
  if (!IsContributeAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  std::unique_ptr<contribute::GetAccessToken::Params> params(
      contribute::GetAccessToken::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto* service = GetContributeService(browser_context());
  bool token_request = service->GetAccessToken(params->code,
      base::BindOnce(
          &ContributeGetAccessTokenFunction::OnCodeResult, this));

  if (!token_request) {
    return RespondNow(
        Error("Could not make request for access tokens"));
  }

  return RespondLater();
}

void ContributeGetAccessTokenFunction::OnCodeResult(bool success) {
  Respond(OneArgument(std::make_unique<base::Value>(success)));
}

ExtensionFunction::ResponseAction
ContributeGetAccountBalancesFunction::Run() {
  if (!IsContributeAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  auto* service = GetContributeService(browser_context());
  bool balance_success = service->GetAccountBalances(
      base::BindOnce(
          &ContributeGetAccountBalancesFunction::OnGetAccountBalances,
          this));

  if (!balance_success) {
    return RespondNow(Error("Could not send request to get balance"));
  }

  return RespondLater();
}

void ContributeGetAccountBalancesFunction::OnGetAccountBalances(
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
ContributeGetConvertQuoteFunction::Run() {
  if (!IsContributeAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  std::unique_ptr<contribute::GetConvertQuote::Params> params(
      contribute::GetConvertQuote::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto* service = GetContributeService(browser_context());
  bool token_request = service->GetConvertQuote(
      params->from, params->to, params->amount,
      base::BindOnce(
          &ContributeGetConvertQuoteFunction::OnQuoteResult, this));

  if (!token_request) {
    return RespondNow(
        Error("Could not make request for access tokens"));
  }

  return RespondLater();
}

void ContributeGetConvertQuoteFunction::OnQuoteResult(
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
ContributeGetTickerPriceFunction::Run() {
  std::unique_ptr<contribute::GetTickerPrice::Params> params(
      contribute::GetTickerPrice::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  if (!IsContributeAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  auto* service = GetContributeService(browser_context());
  bool value_request = service->GetTickerPrice(params->symbol_pair,
      base::BindOnce(
          &ContributeGetTickerPriceFunction::OnGetTickerPrice, this));

  if (!value_request) {
    return RespondNow(
        Error("Could not make request for BTC price"));
  }

  return RespondLater();
}

void ContributeGetTickerPriceFunction::OnGetTickerPrice(
    const std::string& symbol_pair_price) {
  Respond(OneArgument(std::make_unique<base::Value>(symbol_pair_price)));
}

ExtensionFunction::ResponseAction
ContributeGetTickerVolumeFunction::Run() {
  std::unique_ptr<contribute::GetTickerVolume::Params> params(
      contribute::GetTickerVolume::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  if (!IsContributeAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  auto* service = GetContributeService(browser_context());
  bool value_request = service->GetTickerVolume(params->symbol_pair,
      base::BindOnce(
          &ContributeGetTickerVolumeFunction::OnGetTickerVolume, this));

  if (!value_request) {
    return RespondNow(
        Error("Could not make request for Volume"));
  }

  return RespondLater();
}

void ContributeGetTickerVolumeFunction::OnGetTickerVolume(
    const std::string& symbol_pair_volume) {
  Respond(OneArgument(std::make_unique<base::Value>(symbol_pair_volume)));
}

ExtensionFunction::ResponseAction
ContributeIsSupportedRegionFunction::Run() {
  if (!IsContributeAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  bool is_blacklisted = false;
  Profile* profile = Profile::FromBrowserContext(browser_context());
  const int32_t user_country_id =
      country_codes::GetCountryIDFromPrefs(profile->GetPrefs());

  for (const auto& country : ::contribute::kContributeBlacklistRegions) {
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
ContributeGetDepositInfoFunction::Run() {
  if (!IsContributeAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  std::unique_ptr<contribute::GetDepositInfo::Params> params(
      contribute::GetDepositInfo::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto* service = GetContributeService(browser_context());
  bool info_request = service->GetDepositInfo(params->symbol,
      base::BindOnce(
          &ContributeGetDepositInfoFunction::OnGetDepositInfo, this));

  if (!info_request) {
    return RespondNow(
        Error("Could not make request for deposit information."));
  }

  return RespondLater();
}

void ContributeGetDepositInfoFunction::OnGetDepositInfo(
    const std::string& deposit_address,
    const std::string& deposit_url,
    bool success) {
  Respond(TwoArguments(
      std::make_unique<base::Value>(deposit_address),
      std::make_unique<base::Value>(deposit_url)));
}

ExtensionFunction::ResponseAction
ContributeConfirmConvertFunction::Run() {
  if (!IsContributeAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  std::unique_ptr<contribute::ConfirmConvert::Params> params(
      contribute::ConfirmConvert::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto* service = GetContributeService(browser_context());
  bool confirm_request = service->ConfirmConvert(params->quote_id,
      base::BindOnce(
          &ContributeConfirmConvertFunction::OnConfirmConvert, this));

  if (!confirm_request) {
    return RespondNow(
        Error("Could not confirm conversion"));
  }

  return RespondLater();
}

void ContributeConfirmConvertFunction::OnConfirmConvert(
    bool success, const std::string& message) {
  Respond(TwoArguments(
      std::make_unique<base::Value>(success),
      std::make_unique<base::Value>(message)));
}

ExtensionFunction::ResponseAction
ContributeGetConvertAssetsFunction::Run() {
  if (!IsContributeAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  auto* service = GetContributeService(browser_context());
  bool asset_request = service->GetConvertAssets(base::BindOnce(
          &ContributeGetConvertAssetsFunction::OnGetConvertAssets, this));

  if (!asset_request) {
    return RespondNow(
        Error("Could not retrieve supported convert assets"));
  }

  return RespondLater();
}

void ContributeGetConvertAssetsFunction::OnGetConvertAssets(
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

ExtensionFunction::ResponseAction
ContributeRevokeTokenFunction::Run() {
  if (!IsContributeAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  auto* service = GetContributeService(browser_context());
  bool request = service->RevokeToken(base::BindOnce(
          &ContributeRevokeTokenFunction::OnRevokeToken, this));

  if (!request) {
    return RespondNow(
        Error("Could not revoke token"));
  }

  return RespondLater();
}

void ContributeRevokeTokenFunction::OnRevokeToken(bool success) {
  Respond(OneArgument(std::make_unique<base::Value>(success)));
}

}  // namespace api
}  // namespace extensions
