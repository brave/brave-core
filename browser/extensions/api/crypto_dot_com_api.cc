/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/crypto_dot_com_api.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/browser/profiles/profile_util.h"
#include "brave/common/extensions/api/crypto_dot_com.h"
#include "brave/browser/crypto_dot_com/crypto_dot_com_service_factory.h"
#include "brave/components/crypto_dot_com/browser/crypto_dot_com_service.h"
#include "brave/components/crypto_dot_com/browser/regions.h"
#include "brave/components/crypto_dot_com/common/pref_names.h"
#include "brave/components/ntp_widget_utils/browser/ntp_widget_utils_region.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/constants.h"

namespace {

CryptoDotComService* GetCryptoDotComService(content::BrowserContext* context) {
  return CryptoDotComServiceFactory::GetInstance()
      ->GetForProfile(Profile::FromBrowserContext(context));
}

bool IsCryptoDotComAPIAvailable(content::BrowserContext* context) {
  return brave::IsRegularProfile(context);
}

}  // namespace

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction
CryptoDotComGetTickerInfoFunction::Run() {
  if (!IsCryptoDotComAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  std::unique_ptr<crypto_dot_com::GetTickerInfo::Params> params(
      crypto_dot_com::GetTickerInfo::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto* service = GetCryptoDotComService(browser_context());
  bool info_request = service->GetTickerInfo(
      params->asset,
      base::BindOnce(
          &CryptoDotComGetTickerInfoFunction::OnInfoResult, this));

  if (!info_request) {
    return RespondNow(
        Error("Could not make request for ticker info"));
  }

  return RespondLater();
}

void CryptoDotComGetTickerInfoFunction::OnInfoResult(
    const CryptoDotComTickerInfo& info) {
  base::Value result(base::Value::Type::DICTIONARY);

  for (const auto& att : info) {
    result.SetStringKey(att.first, att.second);
  }

  Respond(OneArgument(std::move(result)));
}

ExtensionFunction::ResponseAction
CryptoDotComGetChartDataFunction::Run() {
  if (!IsCryptoDotComAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  std::unique_ptr<crypto_dot_com::GetChartData::Params> params(
      crypto_dot_com::GetChartData::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto* service = GetCryptoDotComService(browser_context());
  bool chart_data_request = service->GetChartData(
      params->asset,
      base::BindOnce(
          &CryptoDotComGetChartDataFunction::OnChartDataResult, this));

  if (!chart_data_request) {
    return RespondNow(
        Error("Could not make request for chart data"));
  }

  return RespondLater();
}

void CryptoDotComGetChartDataFunction::OnChartDataResult(
    const CryptoDotComChartData& data) {
  base::ListValue result;

  for (const auto& data_point : data) {
    auto point = std::make_unique<base::Value>(
      base::Value::Type::DICTIONARY);
    for (const auto& att : data_point) {
      point->SetStringKey(att.first, att.second);
    }
    result.Append(std::move(point));
  }

  Respond(OneArgument(std::move(result)));
}

ExtensionFunction::ResponseAction
CryptoDotComGetSupportedPairsFunction::Run() {
  if (!IsCryptoDotComAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  auto* service = GetCryptoDotComService(browser_context());
  bool supported_pairs_request = service->GetSupportedPairs(
      base::BindOnce(
          &CryptoDotComGetSupportedPairsFunction::OnSupportedPairsResult,
          this));

  if (!supported_pairs_request) {
    return RespondNow(
        Error("Could not make request for supported pairs"));
  }

  return RespondLater();
}

void CryptoDotComGetSupportedPairsFunction::OnSupportedPairsResult(
    const CryptoDotComSupportedPairs& pairs) {
  base::ListValue result;

  for (const auto& pair : pairs) {
    auto instrument = std::make_unique<base::Value>(
      base::Value::Type::DICTIONARY);
    for (const auto& item : pair) {
      instrument->SetStringKey(item.first, item.second);
    }
    result.Append(std::move(instrument));
  }

  Respond(OneArgument(std::move(result)));
}

ExtensionFunction::ResponseAction
CryptoDotComGetAssetRankingsFunction::Run() {
  if (!IsCryptoDotComAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  auto* service = GetCryptoDotComService(browser_context());
  bool asset_rankings_request = service->GetAssetRankings(
      base::BindOnce(
          &CryptoDotComGetAssetRankingsFunction::OnAssetRankingsResult,
          this));

  if (!asset_rankings_request) {
    return RespondNow(
        Error("Could not make request for asset rankings"));
  }

  return RespondLater();
}

void CryptoDotComGetAssetRankingsFunction::OnAssetRankingsResult(
    const CryptoDotComAssetRankings& rankings) {
  base::Value result(base::Value::Type::DICTIONARY);

  for (const auto& ranking : rankings) {
    base::ListValue ranking_list;
    for (const auto& asset : ranking.second) {
      auto asset_dict = std::make_unique<base::Value>(
          base::Value::Type::DICTIONARY);
      for (const auto& att : asset) {
        asset_dict->SetStringKey(att.first, att.second);
      }
      ranking_list.Append(std::move(asset_dict));
    }
    result.SetKey(ranking.first, std::move(ranking_list));
  }

  Respond(OneArgument(std::move(result)));
}

ExtensionFunction::ResponseAction
CryptoDotComIsSupportedFunction::Run() {
  if (!IsCryptoDotComAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());

  bool is_supported = ntp_widget_utils::IsRegionSupported(
      profile->GetPrefs(), ::crypto_dot_com::unsupported_regions, false);

  return RespondNow(OneArgument(base::Value(is_supported)));
}

ExtensionFunction::ResponseAction
CryptoDotComOnBuyCryptoFunction::Run() {
  if (!IsCryptoDotComAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  profile->GetPrefs()->SetBoolean(kCryptoDotComHasBoughtCrypto, true);
  profile->GetPrefs()->SetBoolean(kCryptoDotComHasInteracted, true);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
CryptoDotComOnInteractionFunction::Run() {
  if (!IsCryptoDotComAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  profile->GetPrefs()->SetBoolean(kCryptoDotComHasInteracted, true);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
CryptoDotComGetInteractionsFunction::Run() {
  if (!IsCryptoDotComAPIAvailable(browser_context())) {
    return RespondNow(Error("Not available in Tor/incognito/guest profile"));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  bool has_bought = profile->GetPrefs()->GetBoolean(
      kCryptoDotComHasBoughtCrypto);
  bool has_interacted = profile->GetPrefs()->GetBoolean(
      kCryptoDotComHasInteracted);

  base::DictionaryValue interactions;
  interactions.SetBoolean("boughtCrypto", has_bought);
  interactions.SetBoolean("interacted", has_interacted);

  return RespondNow(OneArgument(std::move(interactions)));
}

}  // namespace api
}  // namespace extensions
