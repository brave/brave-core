/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/meld_integration_service.h"

#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/environment.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/strings/stringprintf.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/browser/meld_integration_response_parser.h"
#include "brave/components/brave_wallet/browser/meld_integration_responses.h"
#include "brave/components/brave_wallet/common/buildflags.h"
#include "brave/components/brave_wallet/common/meld_integration.mojom-forward.h"
#include "brave/components/constants/brave_services_key.h"
#include "brave/components/json/rs/src/lib.rs.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("meld_integration_service", R"(
      semantics {
        sender: "Meld Integration Service"
        description:
          "This service is used to obtain assets prices from"
          "the external Meld API  for the Brave wallet."
        trigger:
          "Triggered by uses of the native Brave wallet."
        data:
          "Meld JSON RPC response bodies."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "You can enable or disable this feature on chrome://flags."
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

base::flat_map<std::string, std::string> MakeMeldApiHeaders() {
  base::flat_map<std::string, std::string> request_headers(
      brave_wallet::MakeBraveServicesKeyHeaders());
  request_headers["accept"] = "application/json";
  request_headers[brave_wallet::kMeldRpcVersionHeader] =
      brave_wallet::kMeldRpcVersion;

  return request_headers;
}
constexpr char kDefaultMeldStatuses[] = "LIVE,RECENTLY_ADDED";

std::optional<std::string> SanitizeJson(const std::string& json) {
  auto converted_json = std::string(
      json::convert_all_numbers_to_string_and_remove_null_values(json, ""));
  if (converted_json.empty()) {
    return std::nullopt;
  }

  return converted_json;
}

GURL AppendFilterParams(
    GURL url,
    const brave_wallet::mojom::MeldFilterPtr& filter,
    std::optional<base::flat_map<std::string, std::string>> def_params) {
  if (def_params) {
    for (const auto& [key, val] : *def_params) {
      url = net::AppendQueryParameter(url, key, val);
    }
  }

  url = net::AppendQueryParameter(
      url, "statuses",
      filter ? filter->statuses.value_or(kDefaultMeldStatuses)
             : kDefaultMeldStatuses);

  if (!filter) {
    return url;
  }

  if (filter->countries) {
    url = net::AppendQueryParameter(url, "countries", *filter->countries);
  }
  if (filter->fiat_currencies) {
    url = net::AppendQueryParameter(url, "fiatCurrencies",
                                    *filter->fiat_currencies);
  }
  if (filter->crypto_currencies) {
    url = net::AppendQueryParameter(url, "cryptoCurrencies",
                                    *filter->crypto_currencies);
  }
  if (filter->crypto_chains) {
    url =
        net::AppendQueryParameter(url, "cryptoChains", *filter->crypto_chains);
  }
  if (filter->service_providers) {
    url = net::AppendQueryParameter(url, "serviceProviders",
                                    *filter->service_providers);
  }
  if (filter->payment_method_types) {
    url = net::AppendQueryParameter(url, "paymentMethodTypes",
                                    *filter->payment_method_types);
  }
  return url;
}

bool NeedsToParseResponse(const int http_error_code) {
  static const base::NoDestructor<std::unordered_set<int>>
      kRespCodesAllowedToContinueParsing({400, 401, 403});
  return kRespCodesAllowedToContinueParsing->contains(http_error_code);
}

void FillCustomerData(
    const brave_wallet::mojom::CryptoWidgetCustomerDataPtr& customer_data,
    base::Value::Dict& cbwr) {
  if (customer_data) {
    if (customer_data->customer) {
      base::Value::Dict co;
      co.Set("email", customer_data->customer->email);
      cbwr.Set("customer", std::move(co));
    }

    if (customer_data->customer_id) {
      cbwr.Set("customerId", customer_data->customer_id.value());
    }

    if (customer_data->external_customer_id) {
      cbwr.Set("externalCustomerId",
               customer_data->external_customer_id.value());
    }

    if (customer_data->external_session_id) {
      cbwr.Set("externalSessionId", customer_data->external_session_id.value());
    }
  }
}

std::optional<base::Value::Dict> GetCryptoBuyWidgetPayload(
    brave_wallet::mojom::CryptoBuySessionDataPtr session_data,
    brave_wallet::mojom::CryptoWidgetCustomerDataPtr customer_data) {
  if (!session_data) {
    return std::nullopt;
  }

  base::Value::Dict cbwr;
  base::Value::Dict cbsd;
  cbsd.Set("countryCode", session_data->country_code);
  cbsd.Set("destinationCurrencyCode", session_data->destination_currency_code);

  if (session_data->lock_fields && session_data->lock_fields->size()) {
    base::Value::List lfs;
    for (const auto& fld : *session_data->lock_fields) {
      lfs.Append(fld);
    }
    cbsd.Set("lockFields", std::move(lfs));
  }

  if (session_data->payment_method_type) {
    cbsd.Set("paymentMethodType", session_data->payment_method_type.value());
  }

  if (session_data->redirect_url) {
    cbsd.Set("redirectUrl", session_data->redirect_url.value());
  }

  cbsd.Set("serviceProvider", session_data->service_provider);
  cbsd.Set("sourceAmount", session_data->source_amount);
  cbsd.Set("sourceCurrencyCode", session_data->source_currency_code);
  cbsd.Set("walletAddress", session_data->wallet_address);

  if (session_data->wallet_tag) {
    cbsd.Set("walletTag", session_data->wallet_tag.value());
  }

  cbwr.Set("sessionData", std::move(cbsd));
  cbwr.Set("sessionType", "BUY");

  FillCustomerData(customer_data, cbwr);

  return cbwr;
}

std::optional<base::Value::Dict> GetCryptoSellWidgetPayload(
    brave_wallet::mojom::CryptoSellSessionDataPtr session_data,
    brave_wallet::mojom::CryptoWidgetCustomerDataPtr customer_data) {
  if (!session_data) {
    return std::nullopt;
  }

  base::Value::Dict cbwr;
  base::Value::Dict cbsd;
  cbsd.Set("countryCode", session_data->country_code);
  cbsd.Set("destinationCurrencyCode", session_data->destination_currency_code);

  if (session_data->lock_fields && session_data->lock_fields->size()) {
    base::Value::List lfs;
    for (const auto& fld : *session_data->lock_fields) {
      lfs.Append(fld);
    }
    cbsd.Set("lockFields", std::move(lfs));
  }

  if (session_data->payment_method_type) {
    cbsd.Set("paymentMethodType", session_data->payment_method_type.value());
  }

  if (session_data->redirect_url) {
    cbsd.Set("redirectUrl", session_data->redirect_url.value());
  }

  cbsd.Set("serviceProvider", session_data->service_provider);
  cbsd.Set("sourceAmount", session_data->source_amount);
  cbsd.Set("sourceCurrencyCode", session_data->source_currency_code);
  if (session_data->wallet_address) {
    cbsd.Set("walletAddress", session_data->wallet_address.value());
  }

  if (session_data->wallet_tag) {
    cbsd.Set("walletTag", session_data->wallet_tag.value());
  }

  cbwr.Set("sessionData", std::move(cbsd));
  cbwr.Set("sessionType", "SELL");

  FillCustomerData(customer_data, cbwr);

  return cbwr;
}

std::optional<base::Value::Dict> GetCryptoTransferWidgetPayload(
    brave_wallet::mojom::CryptoTransferSessionDataPtr session_data,
    brave_wallet::mojom::CryptoWidgetCustomerDataPtr customer_data) {
  if (!session_data) {
    return std::nullopt;
  }

  base::Value::Dict cbwr;
  base::Value::Dict cbsd;

  if (session_data->country_code) {
    cbsd.Set("countryCode", session_data->country_code.value());
  }

  if (session_data->institution_id) {
    cbsd.Set("institutionId", session_data->institution_id.value());
  }

  if (session_data->lock_fields && session_data->lock_fields->size()) {
    base::Value::List lfs;
    for (const auto& fld : *session_data->lock_fields) {
      lfs.Append(fld);
    }
    cbsd.Set("lockFields", std::move(lfs));
  }

  if (session_data->redirect_url) {
    cbsd.Set("redirectUrl", session_data->redirect_url.value());
  }

  cbsd.Set("serviceProvider", session_data->service_provider);

  if (session_data->source_amount) {
    cbsd.Set("sourceAmount", session_data->source_amount.value());
  }

  base::Value::List source_currency_codes;
  for (const auto& cc : session_data->source_currency_codes) {
    source_currency_codes.Append(cc);
  }
  cbsd.Set("sourceCurrencyCodes", std::move(source_currency_codes));

  if (session_data->wallet_address) {
    cbsd.Set("walletAddress", session_data->wallet_address.value());
  }

  if (session_data->wallet_tag) {
    cbsd.Set("walletTag", session_data->wallet_tag.value());
  }

  cbwr.Set("sessionData", std::move(cbsd));
  cbwr.Set("sessionType", "TRANSFER");

  FillCustomerData(customer_data, cbwr);

  return cbwr;
}

std::string GetPayload(const std::optional<base::Value::Dict>& payload_value) {
  if (!payload_value) {
    return "";
  }

  std::string payload;
  auto serialize_success =
      base::JSONWriter::Write(payload_value.value(), &payload);
  DCHECK(serialize_success);
  return payload;
}

}  // namespace

namespace brave_wallet {
MeldIntegrationService::MeldIntegrationService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(
          std::make_unique<api_request_helper::APIRequestHelper>(
              GetNetworkTrafficAnnotationTag(),
              url_loader_factory)) {}

MeldIntegrationService::~MeldIntegrationService() = default;

mojo::PendingRemote<mojom::MeldIntegrationService>
MeldIntegrationService::MakeRemote() {
  mojo::PendingRemote<mojom::MeldIntegrationService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void MeldIntegrationService::Bind(
    mojo::PendingReceiver<mojom::MeldIntegrationService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

// static
GURL MeldIntegrationService::GetServiceProviderURL(
    const mojom::MeldFilterPtr& filter) {
  return AppendFilterParams(
      GURL(kMeldRpcEndpoint).Resolve("/service-providers"), filter,
      base::flat_map<std::string, std::string>{{"accountFilter", "false"}});
}

void MeldIntegrationService::GetServiceProviders(
    mojom::MeldFilterPtr filter,
    GetServiceProvidersCallback callback) {
  auto internal_callback =
      base::BindOnce(&MeldIntegrationService::OnGetServiceProviders,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  auto conversion_callback = base::BindOnce(&SanitizeJson);
  api_request_helper_->Request(
      "GET", GetServiceProviderURL(filter), "", "",
      std::move(internal_callback), MakeMeldApiHeaders(),
      {.auto_retry_on_network_change = true}, std::move(conversion_callback));
}

void MeldIntegrationService::OnGetServiceProviders(
    GetServiceProvidersCallback callback,
    APIRequestResult api_request_result) const {
  if (!api_request_result.Is2XXResponseCode() &&
      !NeedsToParseResponse(api_request_result.response_code())) {
    std::move(callback).Run(
        std::nullopt, std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
    return;
  }

  if (auto errors = ParseMeldErrorResponse(api_request_result.value_body());
      errors) {
    std::move(callback).Run(std::nullopt, errors);
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ParseServiceProviders, api_request_result.TakeBody()),
      base::BindOnce(&MeldIntegrationService::OnParseServiceProviders,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void MeldIntegrationService::OnParseServiceProviders(
    GetServiceProvidersCallback callback,
    std::optional<std::vector<mojom::MeldServiceProviderPtr>> service_providers)
    const {
  if (!service_providers) {
    std::move(callback).Run(
        std::nullopt, std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});
    return;
  }
  std::move(callback).Run(std::move(service_providers), std::nullopt);
}

void MeldIntegrationService::GetCryptoQuotes(
    const std::string& country,
    const std::string& source_currency_code,
    const std::string& destination_currency_code,
    const double source_amount,
    const std::optional<std::string>& account,
    GetCryptoQuotesCallback callback) {
  base::Value::Dict payload;
  AddKeyIfNotEmpty(&payload, "countryCode", country);
  AddKeyIfNotEmpty(&payload, "sourceCurrencyCode", source_currency_code);
  AddKeyIfNotEmpty(&payload, "sourceAmount",
                   base::NumberToString(source_amount));
  AddKeyIfNotEmpty(&payload, "destinationCurrencyCode",
                   destination_currency_code);
  if (account) {
    AddKeyIfNotEmpty(&payload, "walletAddress", *account);
  }

  const std::string json_payload = GetJSON(payload);

  auto url = GURL(kMeldRpcEndpoint).Resolve("/payments/crypto/quote");
  auto internal_callback =
      base::BindOnce(&MeldIntegrationService::OnGetCryptoQuotes,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  auto conversion_callback = base::BindOnce(&SanitizeJson);
  api_request_helper_->Request(
      "POST", url, json_payload, "application/json",
      std::move(internal_callback), MakeMeldApiHeaders(),
      {.auto_retry_on_network_change = true}, std::move(conversion_callback));
}

void MeldIntegrationService::OnGetCryptoQuotes(
    GetCryptoQuotesCallback callback,
    APIRequestResult api_request_result) const {
  if (!api_request_result.Is2XXResponseCode() &&
      !NeedsToParseResponse(api_request_result.response_code())) {
    std::move(callback).Run(
        std::nullopt, std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
    return;
  }

  if (auto errors = ParseMeldErrorResponse(api_request_result.value_body());
      errors) {
    std::move(callback).Run(std::nullopt, errors);
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ParseCryptoQuotes, api_request_result.TakeBody()),
      base::BindOnce(&MeldIntegrationService::OnParseCryptoQuotes,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void MeldIntegrationService::OnParseCryptoQuotes(
    GetCryptoQuotesCallback callback,
    base::expected<std::vector<mojom::MeldCryptoQuotePtr>, std::string>
        quotes_result) const {
  if (!quotes_result.has_value() && quotes_result.error().empty()) {
    std::move(callback).Run(
        std::nullopt, std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});
    return;
  }

  if (!quotes_result.has_value()) {
    std::move(callback).Run(std::nullopt,
                            std::vector<std::string>{quotes_result.error()});
    return;
  }

  std::move(callback).Run(std::move(quotes_result.value()), std::nullopt);
}

// static
GURL MeldIntegrationService::GetPaymentMethodsURL(
    const mojom::MeldFilterPtr& filter) {
  return AppendFilterParams(
      GURL(kMeldRpcEndpoint)
          .Resolve("/service-providers/properties/payment-methods"),
      filter,
      base::flat_map<std::string, std::string>{
          {"includeServiceProviderDetails", "false"},
          {"accountFilter", "false"}});
}

void MeldIntegrationService::GetPaymentMethods(
    mojom::MeldFilterPtr filter,
    GetPaymentMethodsCallback callback) {
  auto internal_callback =
      base::BindOnce(&MeldIntegrationService::OnGetPaymentMethods,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  auto conversion_callback = base::BindOnce(&SanitizeJson);
  api_request_helper_->Request(
      "GET", GetPaymentMethodsURL(filter), "", "", std::move(internal_callback),
      MakeMeldApiHeaders(), {.auto_retry_on_network_change = true},
      std::move(conversion_callback));
}

void MeldIntegrationService::OnGetPaymentMethods(
    GetPaymentMethodsCallback callback,
    APIRequestResult api_request_result) const {
  if (!api_request_result.Is2XXResponseCode() &&
      !NeedsToParseResponse(api_request_result.response_code())) {
    std::move(callback).Run(
        std::nullopt, std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
    return;
  }

  if (auto errors = ParseMeldErrorResponse(api_request_result.value_body());
      errors) {
    std::move(callback).Run(std::nullopt, errors);
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ParsePaymentMethods, api_request_result.TakeBody()),
      base::BindOnce(&MeldIntegrationService::OnParsePaymentMethods,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void MeldIntegrationService::OnParsePaymentMethods(
    GetPaymentMethodsCallback callback,
    std::optional<std::vector<mojom::MeldPaymentMethodPtr>> payment_methods)
    const {
  if (!payment_methods) {
    std::move(callback).Run(
        std::nullopt, std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});
    return;
  }
  std::move(callback).Run(std::move(payment_methods), std::nullopt);
}

// static
GURL MeldIntegrationService::GetFiatCurrenciesURL(
    const mojom::MeldFilterPtr& filter) {
  return AppendFilterParams(
      GURL(kMeldRpcEndpoint)
          .Resolve("/service-providers/properties/fiat-currencies"),
      filter,
      base::flat_map<std::string, std::string>{
          {"includeServiceProviderDetails", "false"},
          {"accountFilter", "false"}});
}

void MeldIntegrationService::GetFiatCurrencies(
    mojom::MeldFilterPtr filter,
    GetFiatCurrenciesCallback callback) {
  auto internal_callback =
      base::BindOnce(&MeldIntegrationService::OnGetFiatCurrencies,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  auto conversion_callback = base::BindOnce(&SanitizeJson);
  api_request_helper_->Request(
      "GET", GetFiatCurrenciesURL(filter), "", "", std::move(internal_callback),
      MakeMeldApiHeaders(), {.auto_retry_on_network_change = true},
      std::move(conversion_callback));
}

void MeldIntegrationService::OnGetFiatCurrencies(
    GetFiatCurrenciesCallback callback,
    APIRequestResult api_request_result) const {
  if (!api_request_result.Is2XXResponseCode() &&
      !NeedsToParseResponse(api_request_result.response_code())) {
    std::move(callback).Run(
        std::nullopt, std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
    return;
  }

  if (auto errors = ParseMeldErrorResponse(api_request_result.value_body());
      errors) {
    std::move(callback).Run(std::nullopt, errors);
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ParseFiatCurrencies, api_request_result.TakeBody()),
      base::BindOnce(&MeldIntegrationService::OnParseFiatCurrencies,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void MeldIntegrationService::OnParseFiatCurrencies(
    GetFiatCurrenciesCallback callback,
    std::optional<std::vector<mojom::MeldFiatCurrencyPtr>> fiat_currencies)
    const {
  if (!fiat_currencies) {
    std::move(callback).Run(
        std::nullopt, std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});
    return;
  }
  std::move(callback).Run(std::move(fiat_currencies), std::nullopt);
}

// static
GURL MeldIntegrationService::GetCryptoCurrenciesURL(
    const mojom::MeldFilterPtr& filter) {
  return AppendFilterParams(
      GURL(kMeldRpcEndpoint)
          .Resolve("/service-providers/properties/crypto-currencies"),
      filter,
      base::flat_map<std::string, std::string>{
          {"includeServiceProviderDetails", "false"},
          {"accountFilter", "false"}});
}

void MeldIntegrationService::GetCryptoCurrencies(
    mojom::MeldFilterPtr filter,
    GetCryptoCurrenciesCallback callback) {
  auto internal_callback =
      base::BindOnce(&MeldIntegrationService::OnGetCryptoCurrencies,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  auto conversion_callback = base::BindOnce(&SanitizeJson);
  api_request_helper_->Request(
      "GET", GetCryptoCurrenciesURL(filter), "", "",
      std::move(internal_callback), MakeMeldApiHeaders(),
      {.auto_retry_on_network_change = true}, std::move(conversion_callback));
}

void MeldIntegrationService::OnGetCryptoCurrencies(
    GetCryptoCurrenciesCallback callback,
    APIRequestResult api_request_result) const {
  if (!api_request_result.Is2XXResponseCode() &&
      !NeedsToParseResponse(api_request_result.response_code())) {
    std::move(callback).Run(
        std::nullopt, std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
    return;
  }

  if (auto errors = ParseMeldErrorResponse(api_request_result.value_body());
      errors) {
    std::move(callback).Run(std::nullopt, errors);
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ParseCryptoCurrencies, api_request_result.TakeBody()),
      base::BindOnce(&MeldIntegrationService::OnParseCryptoCurrencies,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void MeldIntegrationService::OnParseCryptoCurrencies(
    GetCryptoCurrenciesCallback callback,
    std::optional<std::vector<mojom::MeldCryptoCurrencyPtr>> crypto_currencies)
    const {
  if (!crypto_currencies) {
    std::move(callback).Run(
        std::nullopt, std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});
    return;
  }
  std::move(callback).Run(std::move(crypto_currencies), std::nullopt);
}

// static
GURL MeldIntegrationService::GetCountriesURL(
    const mojom::MeldFilterPtr& filter) {
  return AppendFilterParams(
      GURL(kMeldRpcEndpoint).Resolve("/service-providers/properties/countries"),
      filter,
      base::flat_map<std::string, std::string>{
          {"includeServiceProviderDetails", "false"},
          {"accountFilter", "false"}});
}

void MeldIntegrationService::GetCountries(mojom::MeldFilterPtr filter,
                                          GetCountriesCallback callback) {
  auto internal_callback =
      base::BindOnce(&MeldIntegrationService::OnGetCountries,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  auto conversion_callback = base::BindOnce(&SanitizeJson);
  api_request_helper_->Request(
      "GET", GetCountriesURL(filter), "", "", std::move(internal_callback),
      MakeMeldApiHeaders(), {.auto_retry_on_network_change = true},
      std::move(conversion_callback));
}

void MeldIntegrationService::OnGetCountries(
    GetCountriesCallback callback,
    APIRequestResult api_request_result) const {
  if (!api_request_result.Is2XXResponseCode() &&
      !NeedsToParseResponse(api_request_result.response_code())) {
    std::move(callback).Run(
        std::nullopt, std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
    return;
  }

  if (auto errors = ParseMeldErrorResponse(api_request_result.value_body());
      errors) {
    std::move(callback).Run(std::nullopt, errors);
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&ParseCountries, api_request_result.TakeBody()),
      base::BindOnce(&MeldIntegrationService::OnParseCountries,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void MeldIntegrationService::OnParseCountries(
    GetCountriesCallback callback,
    std::optional<std::vector<mojom::MeldCountryPtr>> countries) const {
  if (!countries) {
    std::move(callback).Run(
        std::nullopt, std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});
    return;
  }
  std::move(callback).Run(std::move(countries), std::nullopt);
}

void MeldIntegrationService::CryptoBuyWidgetCreate(
    mojom::CryptoBuySessionDataPtr session_data,
    mojom::CryptoWidgetCustomerDataPtr customer_data,
    CryptoBuyWidgetCreateCallback callback) {
  auto payload_value = GetCryptoBuyWidgetPayload(std::move(session_data),
                                                 std::move(customer_data));
  if (!payload_value) {
    std::move(callback).Run(nullptr,
                            std::vector<std::string>{l10n_util::GetStringUTF8(
                                IDS_WALLET_REQUEST_PROCESSING_ERROR)});
    return;
  }

  auto internal_callback =
      base::BindOnce(&MeldIntegrationService::OnCryptoBuyWidgetCreate,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  auto url = GURL(kMeldRpcEndpoint).Resolve("/crypto/session/widget");
  auto conversion_callback = base::BindOnce(&SanitizeJson);
  api_request_helper_->Request(
      "POST", url, GetPayload(payload_value), "", std::move(internal_callback),
      MakeMeldApiHeaders(), {.auto_retry_on_network_change = true},
      std::move(conversion_callback));
}

void MeldIntegrationService::OnParseCryptoBuyWidgetCreate(
    CryptoBuyWidgetCreateCallback callback,
    mojom::MeldCryptoWidgetPtr crypto_widget) const {
  if (!crypto_widget) {
    std::move(callback).Run(
        nullptr, std::vector<std::string>{
                     l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});
    return;
  }

  std::move(callback).Run(std::move(crypto_widget), std::nullopt);
}

void MeldIntegrationService::OnCryptoBuyWidgetCreate(
    CryptoBuyWidgetCreateCallback callback,
    APIRequestResult api_request_result) const {
  if (!api_request_result.Is2XXResponseCode() &&
      !NeedsToParseResponse(api_request_result.response_code())) {
    std::move(callback).Run(
        nullptr, std::vector<std::string>{
                     l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
    return;
  }

  if (auto errors = ParseMeldErrorResponse(api_request_result.value_body());
      errors) {
    std::move(callback).Run(nullptr, errors);
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ParseCryptoWidgetCreate, api_request_result.TakeBody()),
      base::BindOnce(&MeldIntegrationService::OnParseCryptoBuyWidgetCreate,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void MeldIntegrationService::CryptoSellWidgetCreate(
    mojom::CryptoSellSessionDataPtr session_data,
    mojom::CryptoWidgetCustomerDataPtr customer_data,
    CryptoSellWidgetCreateCallback callback) {
  auto payload_value = GetCryptoSellWidgetPayload(std::move(session_data),
                                                  std::move(customer_data));
  if (!payload_value) {
    std::move(callback).Run(nullptr,
                            std::vector<std::string>{l10n_util::GetStringUTF8(
                                IDS_WALLET_REQUEST_PROCESSING_ERROR)});
    return;
  }

  auto internal_callback =
      base::BindOnce(&MeldIntegrationService::OnCryptoSellWidgetCreate,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  auto url = GURL(kMeldRpcEndpoint).Resolve("/crypto/session/widget");
  auto conversion_callback = base::BindOnce(&SanitizeJson);
  api_request_helper_->Request(
      "POST", url, GetPayload(payload_value), "", std::move(internal_callback),
      MakeMeldApiHeaders(), {.auto_retry_on_network_change = true},
      std::move(conversion_callback));
}

void MeldIntegrationService::OnParseCryptoSellWidgetCreate(
    CryptoSellWidgetCreateCallback callback,
    mojom::MeldCryptoWidgetPtr crypto_widget) const {
  if (!crypto_widget) {
    std::move(callback).Run(
        nullptr, std::vector<std::string>{
                     l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});
    return;
  }

  std::move(callback).Run(std::move(crypto_widget), std::nullopt);
}

void MeldIntegrationService::OnCryptoSellWidgetCreate(
    CryptoSellWidgetCreateCallback callback,
    APIRequestResult api_request_result) const {
  if (!api_request_result.Is2XXResponseCode() &&
      !NeedsToParseResponse(api_request_result.response_code())) {
    std::move(callback).Run(
        nullptr, std::vector<std::string>{
                     l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
    return;
  }

  if (auto errors = ParseMeldErrorResponse(api_request_result.value_body());
      errors) {
    std::move(callback).Run(nullptr, errors);
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ParseCryptoWidgetCreate, api_request_result.TakeBody()),
      base::BindOnce(&MeldIntegrationService::OnParseCryptoSellWidgetCreate,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void MeldIntegrationService::CryptoTransferWidgetCreate(
    mojom::CryptoTransferSessionDataPtr session_data,
    mojom::CryptoWidgetCustomerDataPtr customer_data,
    CryptoTransferWidgetCreateCallback callback) {
  auto payload_value = GetCryptoTransferWidgetPayload(std::move(session_data),
                                                      std::move(customer_data));
  if (!payload_value) {
    std::move(callback).Run(nullptr,
                            std::vector<std::string>{l10n_util::GetStringUTF8(
                                IDS_WALLET_REQUEST_PROCESSING_ERROR)});
    return;
  }

  auto internal_callback =
      base::BindOnce(&MeldIntegrationService::OnCryptoTransferWidgetCreate,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  auto url = GURL(kMeldRpcEndpoint).Resolve("/crypto/session/widget");
  auto conversion_callback = base::BindOnce(&SanitizeJson);
  api_request_helper_->Request(
      "POST", url, GetPayload(payload_value), "", std::move(internal_callback),
      MakeMeldApiHeaders(), {.auto_retry_on_network_change = true},
      std::move(conversion_callback));
}

void MeldIntegrationService::OnParseCryptoTransferWidgetCreate(
    CryptoTransferWidgetCreateCallback callback,
    mojom::MeldCryptoWidgetPtr crypto_widget) const {
  if (!crypto_widget) {
    std::move(callback).Run(
        nullptr, std::vector<std::string>{
                     l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});
    return;
  }

  std::move(callback).Run(std::move(crypto_widget), std::nullopt);
}

void MeldIntegrationService::OnCryptoTransferWidgetCreate(
    CryptoTransferWidgetCreateCallback callback,
    APIRequestResult api_request_result) const {
  if (!api_request_result.Is2XXResponseCode() &&
      !NeedsToParseResponse(api_request_result.response_code())) {
    std::move(callback).Run(
        nullptr, std::vector<std::string>{
                     l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
    return;
  }

  if (auto errors = ParseMeldErrorResponse(api_request_result.value_body());
      errors) {
    std::move(callback).Run(nullptr, errors);
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ParseCryptoWidgetCreate, api_request_result.TakeBody()),
      base::BindOnce(&MeldIntegrationService::OnParseCryptoTransferWidgetCreate,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

}  // namespace brave_wallet
