/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/meld_integration_service.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/environment.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/browser/meld_integration_response_parser.h"
#include "brave/components/brave_wallet/common/buildflags.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("meld_integration_service", R"(
      semantics {
        sender: "Meld Integration Service"
        description:
          "This service is used to obtain assets prices from the external
 Meld API  for the Brave wallet."
        trigger:
          "Triggered by uses of the native Brave wallet."
        data:
          "Ethereum JSON RPC response bodies."
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
  base::flat_map<std::string, std::string> request_headers;
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  std::string meld_api_key(BUILDFLAG(MELD_API_KEY));
  if (env->HasVar("MELD_API_KEY")) {
    env->GetVar("MELD_API_KEY", &meld_api_key);
  }
  meld_api_key = base::StrCat({"BASIC", " ", meld_api_key});
  request_headers["Authorization"] = std::move(meld_api_key);
  request_headers["accept"] = "application/json";

  return request_headers;
}
constexpr char kDefaultMeldStatuses[] = "LIVE,RECENTLY_ADDED";

}  // namespace

namespace brave_wallet {
using std::move;

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
    const std::optional<std::string>& countries,
    const std::optional<std::string>& fiat_currencies,
    const std::optional<std::string>& crypto_currencies,
    const std::optional<std::string>& service_providers,
    const std::optional<std::string>& payment_method_types,
    const std::optional<std::string>& statuses) {
  auto url = GURL(base::StringPrintf("%s/service-providers",
                                     GetMeldAssetRatioBaseURL().c_str()));
  url = net::AppendQueryParameter(url, "accountFilter", "false");

  url = net::AppendQueryParameter(url, "statuses",
                                  statuses.value_or(kDefaultMeldStatuses));

  if (countries) {
    url = net::AppendQueryParameter(url, "countries", *countries);
  }
  if (fiat_currencies) {
    url = net::AppendQueryParameter(url, "fiatCurrencies", *fiat_currencies);
  }
  if (crypto_currencies) {
    url =
        net::AppendQueryParameter(url, "cryptoCurrencies", *crypto_currencies);
  }
  if (service_providers) {
    url =
        net::AppendQueryParameter(url, "serviceProviders", *service_providers);
  }
  if (payment_method_types) {
    url = net::AppendQueryParameter(url, "paymentMethodTypes",
                                    *payment_method_types);
  }

  return url;
}

void MeldIntegrationService::GetServiceProviders(
    const std::optional<std::string>& countries,
    const std::optional<std::string>& fiat_currencies,
    const std::optional<std::string>& crypto_currencies,
    const std::optional<std::string>& service_providers,
    const std::optional<std::string>& payment_method_types,
    const std::optional<std::string>& statuses,
    GetServiceProvidersCallback callback) {
  auto internal_callback =
      base::BindOnce(&MeldIntegrationService::OnGetServiceProviders,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  api_request_helper_->Request(
      "GET",
      GetServiceProviderURL(countries, fiat_currencies, crypto_currencies,
                            service_providers, payment_method_types, statuses),
      "", "", std::move(internal_callback), MakeMeldApiHeaders(),
      {.auto_retry_on_network_change = true, .enable_cache = true});
}

void MeldIntegrationService::OnGetServiceProviders(
    GetServiceProvidersCallback callback,
    APIRequestResult api_request_result) const {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(std::nullopt,
                            std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
    return;
  }

  if (auto errors = ParseMeldErrorResponse(api_request_result.value_body());
      errors) {
    std::move(callback).Run(std::nullopt, errors);
    return;
  }

  auto service_providers = ParseServiceProviders(api_request_result.value_body());
  if (!service_providers) {
    std::move(callback).Run(std::nullopt, std::vector<std::string>{"PARSING_ERROR"});
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

  auto url = GURL(base::StringPrintf("%s/payments/crypto/quote",
                                     GetMeldAssetRatioBaseURL().c_str()));
  auto internal_callback =
      base::BindOnce(&MeldIntegrationService::OnGetCryptoQuotes,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  api_request_helper_->Request(
      "POST", url, json_payload, "application/json",
      std::move(internal_callback), MakeMeldApiHeaders(),
      {.auto_retry_on_network_change = true, .enable_cache = false});
}

void MeldIntegrationService::OnGetCryptoQuotes(
    GetCryptoQuotesCallback callback,
    APIRequestResult api_request_result) const {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(std::nullopt,
                            std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
    return;
  }

  if (auto errors = ParseMeldErrorResponse(api_request_result.value_body());
      errors) {
    std::move(callback).Run(std::nullopt, errors);
    return;
  }

  std::optional<std::vector<std::string>> errors;
  std::string error;
  auto quotes = ParseCryptoQuotes(api_request_result.value_body(), &error);
  if (!quotes) {
    errors = std::vector<std::string>{"PARSING_ERROR"};
    std::move(callback).Run(std::nullopt, errors);
    return;
  }

  if (!error.empty()) {
    errors = std::vector<std::string>{error};
  }

  std::move(callback).Run(std::move(quotes), errors);
}

// static
GURL MeldIntegrationService::GetPaymentMethodsURL(
    const std::optional<std::string>& countries,
    const std::optional<std::string>& fiat_currencies,
    const std::optional<std::string>& crypto_currencies,
    const std::optional<std::string>& service_providers,
    const std::optional<std::string>& payment_method_types,
    const std::optional<std::string>& statuses) {
  auto url =
      GURL(base::StringPrintf("%s/service-providers/properties/payment-methods",
                              GetMeldAssetRatioBaseURL().c_str()));
  url =
      net::AppendQueryParameter(url, "includeServiceProviderDetails", "false");

  url = net::AppendQueryParameter(url, "statuses",
                                  statuses.value_or(kDefaultMeldStatuses));

  if (countries) {
    url = net::AppendQueryParameter(url, "countries", *countries);
  }
  if (fiat_currencies) {
    url = net::AppendQueryParameter(url, "fiatCurrencies", *fiat_currencies);
  }
  if (crypto_currencies) {
    url =
        net::AppendQueryParameter(url, "cryptoCurrencies", *crypto_currencies);
  }
  if (service_providers) {
    url =
        net::AppendQueryParameter(url, "serviceProviders", *service_providers);
  }
  if (payment_method_types) {
    url = net::AppendQueryParameter(url, "paymentMethodTypes",
                                    *payment_method_types);
  }

  return url;
}

void MeldIntegrationService::GetPaymentMethods(
    const std::optional<std::string>& countries,
    const std::optional<std::string>& fiat_currencies,
    const std::optional<std::string>& crypto_currencies,
    const std::optional<std::string>& service_providers,
    const std::optional<std::string>& payment_method_types,
    const std::optional<std::string>& statuses,
    GetPaymentMethodsCallback callback) {
  auto internal_callback =
      base::BindOnce(&MeldIntegrationService::OnGetPaymentMethods,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  api_request_helper_->Request(
      "GET",
      GetPaymentMethodsURL(countries, fiat_currencies, crypto_currencies,
                           service_providers, payment_method_types, statuses),
      "", "", std::move(internal_callback), MakeMeldApiHeaders(),
      {.auto_retry_on_network_change = true, .enable_cache = true});
}

void MeldIntegrationService::OnGetPaymentMethods(
    GetPaymentMethodsCallback callback,
    APIRequestResult api_request_result) const {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(std::nullopt,
                            std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
    return;
  }

  if (auto errors = ParseMeldErrorResponse(api_request_result.value_body());
      errors) {
    std::move(callback).Run(std::nullopt, errors);
    return;
  }

  auto payment_methods = ParsePaymentMethods(api_request_result.value_body());
  if (!payment_methods) {
    std::move(callback).Run(std::nullopt, std::vector<std::string>{"PARSING_ERROR"});
    return;
  }

  std::move(callback).Run(std::move(payment_methods), std::nullopt);
}

// static
GURL MeldIntegrationService::GetFiatCurrenciesURL(
    const std::optional<std::string>& countries,
    const std::optional<std::string>& fiat_currencies,
    const std::optional<std::string>& crypto_currencies,
    const std::optional<std::string>& service_providers,
    const std::optional<std::string>& payment_method_types,
    const std::optional<std::string>& statuses) {
  auto url =
      GURL(base::StringPrintf("%s/service-providers/properties/fiat-currencies",
                              GetMeldAssetRatioBaseURL().c_str()));
  url =
      net::AppendQueryParameter(url, "includeServiceProviderDetails", "false");

  url = net::AppendQueryParameter(url, "statuses",
                                  statuses.value_or(kDefaultMeldStatuses));

  if (countries) {
    url = net::AppendQueryParameter(url, "countries", *countries);
  }
  if (fiat_currencies) {
    url = net::AppendQueryParameter(url, "fiatCurrencies", *fiat_currencies);
  }
  if (crypto_currencies) {
    url =
        net::AppendQueryParameter(url, "cryptoCurrencies", *crypto_currencies);
  }
  if (service_providers) {
    url =
        net::AppendQueryParameter(url, "serviceProviders", *service_providers);
  }
  if (payment_method_types) {
    url = net::AppendQueryParameter(url, "paymentMethodTypes",
                                    *payment_method_types);
  }

  return url;
}

void MeldIntegrationService::GetFiatCurrencies(
    const std::optional<std::string>& countries,
    const std::optional<std::string>& fiat_currencies,
    const std::optional<std::string>& crypto_currencies,
    const std::optional<std::string>& service_providers,
    const std::optional<std::string>& payment_method_types,
    const std::optional<std::string>& statuses,
    GetFiatCurrenciesCallback callback) {
  auto internal_callback =
      base::BindOnce(&MeldIntegrationService::OnGetFiatCurrencies,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  api_request_helper_->Request(
      "GET",
      GetFiatCurrenciesURL(countries, fiat_currencies, crypto_currencies,
                           service_providers, payment_method_types, statuses),
      "", "", std::move(internal_callback), MakeMeldApiHeaders(),
      {.auto_retry_on_network_change = true, .enable_cache = true});
}

void MeldIntegrationService::OnGetFiatCurrencies(
    GetFiatCurrenciesCallback callback,
    APIRequestResult api_request_result) const {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(std::nullopt,
                            std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
    return;
  }

  if (auto errors = ParseMeldErrorResponse(api_request_result.value_body());
      errors) {
    std::move(callback).Run(std::nullopt, errors);
    return;
  }

  auto fiat_currencies = ParseFiatCurrencies(api_request_result.value_body());
  if (!fiat_currencies) {
    std::move(callback).Run(std::nullopt, std::vector<std::string>{"PARSING_ERROR"});
    return;
  }

  std::move(callback).Run(std::move(fiat_currencies), std::nullopt);
}

// static
GURL MeldIntegrationService::GetCryptoCurrenciesURL(
    const std::optional<std::string>& countries,
    const std::optional<std::string>& fiat_currencies,
    const std::optional<std::string>& crypto_currencies,
    const std::optional<std::string>& service_providers,
    const std::optional<std::string>& payment_method_types,
    const std::optional<std::string>& statuses) {
  auto url = GURL(
      base::StringPrintf("%s/service-providers/properties/crypto-currencies",
                         GetMeldAssetRatioBaseURL().c_str()));

  url =
      net::AppendQueryParameter(url, "includeServiceProviderDetails", "false");

  url = net::AppendQueryParameter(url, "statuses",
                                  statuses.value_or(kDefaultMeldStatuses));

  if (countries) {
    url = net::AppendQueryParameter(url, "countries", *countries);
  }
  if (fiat_currencies) {
    url = net::AppendQueryParameter(url, "fiatCurrencies", *fiat_currencies);
  }
  if (crypto_currencies) {
    url =
        net::AppendQueryParameter(url, "cryptoCurrencies", *crypto_currencies);
  }
  if (service_providers) {
    url =
        net::AppendQueryParameter(url, "serviceProviders", *service_providers);
  }
  if (payment_method_types) {
    url = net::AppendQueryParameter(url, "paymentMethodTypes",
                                    *payment_method_types);
  }

  return url;
}

void MeldIntegrationService::GetCryptoCurrencies(
    const std::optional<std::string>& countries,
    const std::optional<std::string>& fiat_currencies,
    const std::optional<std::string>& crypto_currencies,
    const std::optional<std::string>& service_providers,
    const std::optional<std::string>& payment_method_types,
    const std::optional<std::string>& statuses,
    GetCryptoCurrenciesCallback callback) {
  auto internal_callback =
      base::BindOnce(&MeldIntegrationService::OnGetCryptoCurrencies,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  api_request_helper_->Request(
      "GET",
      GetCryptoCurrenciesURL(countries, fiat_currencies, crypto_currencies,
                             service_providers, payment_method_types, statuses),
      "", "", std::move(internal_callback), MakeMeldApiHeaders(),
      {.auto_retry_on_network_change = true, .enable_cache = true});
}

void MeldIntegrationService::OnGetCryptoCurrencies(
    GetCryptoCurrenciesCallback callback,
    APIRequestResult api_request_result) const {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(std::nullopt,
                            std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
    return;
  }

  if (auto errors = ParseMeldErrorResponse(api_request_result.value_body());
      errors) {
    std::move(callback).Run(std::nullopt, errors);
    return;
  }

  auto crypto_currencies = ParseCryptoCurrencies(api_request_result.value_body());
  if (!crypto_currencies) {
    std::move(callback).Run(std::nullopt, std::vector<std::string>{"PARSING_ERROR"});
    return;
  }

  std::move(callback).Run(std::move(crypto_currencies), std::nullopt);
}

// static
GURL MeldIntegrationService::GetCountriesURL(
    const std::optional<std::string>& countries,
    const std::optional<std::string>& fiat_currencies,
    const std::optional<std::string>& crypto_currencies,
    const std::optional<std::string>& service_providers,
    const std::optional<std::string>& payment_method_types,
    const std::optional<std::string>& statuses) {
  auto url =
      GURL(base::StringPrintf("%s/service-providers/properties/countries",
                              GetMeldAssetRatioBaseURL().c_str()));

  url =
      net::AppendQueryParameter(url, "includeServiceProviderDetails", "false");

  url = net::AppendQueryParameter(url, "statuses",
                                  statuses.value_or(kDefaultMeldStatuses));

  if (countries) {
    url = net::AppendQueryParameter(url, "countries", *countries);
  }
  if (fiat_currencies) {
    url = net::AppendQueryParameter(url, "fiatCurrencies", *fiat_currencies);
  }
  if (crypto_currencies) {
    url =
        net::AppendQueryParameter(url, "cryptoCurrencies", *crypto_currencies);
  }
  if (service_providers) {
    url =
        net::AppendQueryParameter(url, "serviceProviders", *service_providers);
  }
  if (payment_method_types) {
    url = net::AppendQueryParameter(url, "paymentMethodTypes",
                                    *payment_method_types);
  }

  return url;
}

void MeldIntegrationService::GetCountries(
    const std::optional<std::string>& countries,
    const std::optional<std::string>& fiat_currencies,
    const std::optional<std::string>& crypto_currencies,
    const std::optional<std::string>& service_providers,
    const std::optional<std::string>& payment_method_types,
    const std::optional<std::string>& statuses,
    GetCountriesCallback callback) {
  auto internal_callback =
      base::BindOnce(&MeldIntegrationService::OnGetCountries,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  api_request_helper_->Request(
      "GET",
      GetCountriesURL(countries, fiat_currencies, crypto_currencies,
                      service_providers, payment_method_types, statuses),
      "", "", std::move(internal_callback), MakeMeldApiHeaders(),
      {.auto_retry_on_network_change = true, .enable_cache = true});
}

void MeldIntegrationService::OnGetCountries(
    GetCountriesCallback callback,
    APIRequestResult api_request_result) const {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(std::nullopt,
                            std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
    return;
  }

  if (auto errors = ParseMeldErrorResponse(api_request_result.value_body());
      errors) {
    std::move(callback).Run(std::nullopt, errors);
    return;
  }

  auto countries = ParseCountries(api_request_result.value_body());
  if (!countries) {
    std::move(callback).Run(std::nullopt, std::vector<std::string>{"PARSING_ERROR"});
    return;
  }
  std::move(callback).Run(std::move(countries), std::nullopt);
}

}  // namespace brave_wallet
