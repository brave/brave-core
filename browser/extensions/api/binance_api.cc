/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/binance_api.h"

#include <memory>
#include <string>
#include <utility>

#include "base/environment.h"
#include "brave/browser/profiles/profile_util.h"

#include "brave/common/extensions/api/binance.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/common/pref_names.h"
#include "brave/browser/binance/binance_service_factory.h"
#include "brave/components/binance/browser/binance_controller.h"
#include "brave/components/binance/browser/binance_service.h"
#include "brave/components/binance/browser/static_values.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extension_util.h"

namespace {

BinanceController* GetBinanceController(content::BrowserContext* context) {
  return BinanceServiceFactory::GetInstance()
      ->GetForProfile(Profile::FromBrowserContext(context))
      ->controller();
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

  auto* controller = GetBinanceController(browser_context());
  const std::string userTLD = controller->GetBinanceTLD();

  return RespondNow(OneArgument(
      std::make_unique<base::Value>(userTLD)));
}

ExtensionFunction::ResponseAction
BinanceGetClientUrlFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (brave::IsTorProfile(profile)) {
    return RespondNow(Error("Not available in Tor profile"));
  }

  auto* controller = GetBinanceController(browser_context());
  const std::string client_url = controller->GetOAuthClientUrl();

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

  auto* controller = GetBinanceController(browser_context());
  bool token_request = controller->GetAccessToken(params->code,
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
BinanceIsSupportedRegionFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (brave::IsTorProfile(profile)) {
    return RespondNow(Error("Not available in Tor profile"));
  }

  bool is_blacklisted = false;
  const int32_t user_country_id =
      country_codes::GetCountryIDFromPrefs(profile->GetPrefs());

  for (const auto& country : binance::kBinanceBlacklistRegions) {
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

}  // namespace api
}  // namespace extensions
