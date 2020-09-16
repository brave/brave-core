/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/crypto_dot_com_api.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/common/extensions/api/crypto_dot_com.h"
#include "brave/browser/crypto_dot_com/crypto_dot_com_service_factory.h"
#include "brave/components/crypto_dot_com/browser/crypto_dot_com_service.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/constants.h"

namespace {

CryptoDotComService* GetCryptoDotComService(content::BrowserContext* context) {
  return CryptoDotComServiceFactory::GetInstance()
      ->GetForProfile(Profile::FromBrowserContext(context));
}

}  // namespace

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction
CryptoDotComGetTickerInfoFunction::Run() {
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
    const std::map<std::string, std::string>& info) {
  auto result = std::make_unique<base::Value>(
      base::Value::Type::DICTIONARY);

  for (const auto& att : info) {
    result->SetStringKey(att.first, att.second);
  }

  Respond(OneArgument(std::move(result)));
}

ExtensionFunction::ResponseAction
CryptoDotComGetChartDataFunction::Run() {
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
    const std::vector<std::map<std::string, std::string>>& data) {
  auto result = std::make_unique<base::ListValue>();

  for (const auto& data_point : data) {
    auto point = std::make_unique<base::Value>(
      base::Value::Type::DICTIONARY);
    for (const auto& att : data_point) {
      point->SetStringKey(att.first, att.second);
    }
    result->Append(std::move(point));
  }

  Respond(OneArgument(std::move(result)));
}

ExtensionFunction::ResponseAction
CryptoDotComGetSupportedPairsFunction::Run() {
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
    const std::vector<std::map<std::string, std::string>>& pairs) {
  auto result = std::make_unique<base::ListValue>();

  for (const auto& pair : pairs) {
    auto instrument = std::make_unique<base::Value>(
      base::Value::Type::DICTIONARY);
    for (const auto& item : pair) {
      instrument->SetStringKey(item.first, item.second);
    }
    result->Append(std::move(instrument));
  }

  Respond(OneArgument(std::move(result)));
}

ExtensionFunction::ResponseAction
CryptoDotComGetAssetRankingsFunction::Run() {
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
    const std::map<std::string,
    std::vector<std::map<std::string, std::string>>>& rankings) {
  auto result = std::make_unique<base::Value>(
      base::Value::Type::DICTIONARY);

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
    result->SetKey(ranking.first, std::move(ranking_list));
  }

  Respond(OneArgument(std::move(result)));
}

}  // namespace api
}  // namespace extensions
