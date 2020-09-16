/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/crypto_dot_com_api.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

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
  /*
  auto* service = GetCryptoDotComService(browser_context());
  bool chart_data_request = service->GetChartData(
      params->asset,
      base::BindOnce(
          &CryptoDotComGetChartDataFunction::OnChartDataResult, this));

  if (!chart_data_request) {
    return RespondNow(
        Error("Could not make request for chart data"));
  }
  */
  return RespondLater();
}

void CryptoDotComGetChartDataFunction::OnChartDataResult(
    const std::map<std::string, std::string>& data) {
  auto result = std::make_unique<base::Value>(
      base::Value::Type::DICTIONARY);

  for (const auto& att : data) {
    result->SetStringKey(att.first, att.second);
  }

  Respond(OneArgument(std::move(result)));
}

}  // namespace api
}  // namespace extensions
