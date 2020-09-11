/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/gemini_api.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "brave/common/extensions/api/gemini.h"
#include "brave/browser/gemini/gemini_service_factory.h"
#include "brave/components/ntp_widget_utils/browser/ntp_widget_utils_region.h"
#include "brave/components/gemini/browser/gemini_service.h"
#include "brave/components/gemini/browser/regions.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/constants.h"

namespace {

GeminiService* GetGeminiService(content::BrowserContext* context) {
  return GeminiServiceFactory::GetInstance()
      ->GetForProfile(Profile::FromBrowserContext(context));
}

}  // namespace

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction
GeminiGetClientUrlFunction::Run() {
  auto* service = GetGeminiService(browser_context());
  const std::string client_url = service->GetOAuthClientUrl();

  return RespondNow(OneArgument(
      std::make_unique<base::Value>(client_url)));
}

ExtensionFunction::ResponseAction
GeminiGetAccessTokenFunction::Run() {
  auto* service = GetGeminiService(browser_context());
  bool token_request = service->GetAccessToken(base::BindOnce(
      &GeminiGetAccessTokenFunction::OnCodeResult, this));

  if (!token_request) {
    return RespondNow(
        Error("Could not make request for access tokens"));
  }

  return RespondLater();
}

void GeminiGetAccessTokenFunction::OnCodeResult(bool success) {
  Respond(OneArgument(std::make_unique<base::Value>(success)));
}

ExtensionFunction::ResponseAction
GeminiRefreshAccessTokenFunction::Run() {
  auto* service = GetGeminiService(browser_context());
  bool token_request = service->RefreshAccessToken(base::BindOnce(
      &GeminiRefreshAccessTokenFunction::OnRefreshResult, this));

  if (!token_request) {
    return RespondNow(
        Error("Could not make request to refresh access tokens"));
  }

  return RespondLater();
}

void GeminiRefreshAccessTokenFunction::OnRefreshResult(bool success) {
  Respond(OneArgument(std::make_unique<base::Value>(success)));
}

ExtensionFunction::ResponseAction
GeminiGetTickerPriceFunction::Run() {
  std::unique_ptr<gemini::GetTickerPrice::Params> params(
      gemini::GetTickerPrice::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto* service = GetGeminiService(browser_context());
  bool price_request = service->GetTickerPrice(
      params->asset,
      base::BindOnce(
          &GeminiGetTickerPriceFunction::OnPriceResult, this));

  if (!price_request) {
    return RespondNow(
        Error("Could not make request for price"));
  }

  return RespondLater();
}

void GeminiGetTickerPriceFunction::OnPriceResult(
    const std::string& price) {
  Respond(OneArgument(std::make_unique<base::Value>(price)));
}

ExtensionFunction::ResponseAction
GeminiGetAccountBalancesFunction::Run() {
  auto* service = GetGeminiService(browser_context());
  bool balance_success = service->GetAccountBalances(
      base::BindOnce(
          &GeminiGetAccountBalancesFunction::OnGetAccountBalances,
          this));

  if (!balance_success) {
    return RespondNow(Error("Could not send request to get balance"));
  }

  return RespondLater();
}

void GeminiGetAccountBalancesFunction::OnGetAccountBalances(
    const std::map<std::string, std::string>& balances,
    bool auth_invalid) {
  auto result = std::make_unique<base::Value>(
      base::Value::Type::DICTIONARY);

  for (const auto& balance : balances) {
    result->SetStringKey(balance.first, balance.second);
  }

  Respond(TwoArguments(std::move(result),
                       std::make_unique<base::Value>(auth_invalid)));
}

ExtensionFunction::ResponseAction
GeminiGetDepositInfoFunction::Run() {
  std::unique_ptr<gemini::GetDepositInfo::Params> params(
      gemini::GetDepositInfo::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto* service = GetGeminiService(browser_context());
  bool info_request = service->GetDepositInfo(params->asset,
      base::BindOnce(
          &GeminiGetDepositInfoFunction::OnGetDepositInfo, this));

  if (!info_request) {
    return RespondNow(
        Error("Could not make request for deposit information."));
  }

  return RespondLater();
}

void GeminiGetDepositInfoFunction::OnGetDepositInfo(
    const std::string& deposit_address) {
  Respond(OneArgument(
      std::make_unique<base::Value>(deposit_address)));
}

ExtensionFunction::ResponseAction
GeminiRevokeTokenFunction::Run() {
  auto* service = GetGeminiService(browser_context());
  bool request = service->RevokeAccessToken(base::BindOnce(
          &GeminiRevokeTokenFunction::OnRevokeToken, this));

  if (!request) {
    return RespondNow(
        Error("Could not revoke gemini access tokens"));
  }

  return RespondLater();
}

void GeminiRevokeTokenFunction::OnRevokeToken(bool success) {
  Respond(OneArgument(std::make_unique<base::Value>(success)));
}

ExtensionFunction::ResponseAction
GeminiGetOrderQuoteFunction::Run() {
  std::unique_ptr<gemini::GetOrderQuote::Params> params(
      gemini::GetOrderQuote::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto* service = GetGeminiService(browser_context());
  bool quote_request = service->GetOrderQuote(
      params->side, params->symbol, params->spend,
      base::BindOnce(
          &GeminiGetOrderQuoteFunction::OnOrderQuoteResult, this));

  if (!quote_request) {
    return RespondNow(
        Error("Could not make request for quote"));
  }

  return RespondLater();
}

void GeminiGetOrderQuoteFunction::OnOrderQuoteResult(
    const std::string& quote_id, const std::string& quantity,
    const std::string& fee, const std::string& price,
    const std::string& total_price, const std::string& error) {
  auto quote = std::make_unique<base::Value>(base::Value::Type::DICTIONARY);
  quote->SetStringKey("id", quote_id);
  quote->SetStringKey("quantity", quantity);
  quote->SetStringKey("fee", fee);
  quote->SetStringKey("price", price);
  quote->SetStringKey("totalPrice", total_price);
  Respond(TwoArguments(
    std::move(quote), std::make_unique<base::Value>(error)));
}

ExtensionFunction::ResponseAction
GeminiExecuteOrderFunction::Run() {
  std::unique_ptr<gemini::ExecuteOrder::Params> params(
      gemini::ExecuteOrder::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto* service = GetGeminiService(browser_context());
  bool balance_success = service->ExecuteOrder(
      params->symbol, params->side, params->quantity,
      params->price, params->fee, params->quote_id,
      base::BindOnce(
          &GeminiExecuteOrderFunction::OnOrderExecuted,
          this));

  if (!balance_success) {
    return RespondNow(Error("Could not send request to execute order"));
  }

  return RespondLater();
}

void GeminiExecuteOrderFunction::OnOrderExecuted(bool success) {
  Respond(OneArgument(std::make_unique<base::Value>(success)));
}

ExtensionFunction::ResponseAction
GeminiIsSupportedFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  bool is_supported = ntp_widget_utils::IsRegionSupported(
      profile->GetPrefs(), ::gemini::supported_regions, true);
  return RespondNow(OneArgument(
      std::make_unique<base::Value>(is_supported)));
}

}  // namespace api
}  // namespace extensions
