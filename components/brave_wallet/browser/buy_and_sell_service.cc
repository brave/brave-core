/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/buy_and_sell_service.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/environment.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/buy_and_sell_response_parser.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/common/buildflags.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("buy_and_sell_service", R"(
      semantics {
        sender: "Buy And Sell Service"
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

BuyAndSellService::BuyAndSellService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(
          std::make_unique<api_request_helper::APIRequestHelper>(
              GetNetworkTrafficAnnotationTag(),
              url_loader_factory)),
      weak_ptr_factory_(this) {}

BuyAndSellService::~BuyAndSellService() = default;

mojo::PendingRemote<mojom::BuyAndSellService> BuyAndSellService::MakeRemote() {
  mojo::PendingRemote<mojom::BuyAndSellService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void BuyAndSellService::Bind(
    mojo::PendingReceiver<mojom::BuyAndSellService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

// static
GURL BuyAndSellService::GetServiceProviderURL(
    const std::string& countries,
    const std::string& fiat_currencies,
    const std::string& crypto_currencies,
    const std::string& service_providers,
    const std::string& payment_method_types,
    const std::string& statuses) {
  auto url = GURL(base::StringPrintf("%s/service-providers",
                                     GetMeldAssetRatioBaseURL().c_str()));
  url = net::AppendQueryParameter(url, "accountFilter", "false");
  if (!statuses.empty()) {
    url = net::AppendQueryParameter(url, "statuses", statuses);
  } else {
    url = net::AppendQueryParameter(url, "statuses", kDefaultMeldStatuses);
  }

  if (!countries.empty()) {
    url = net::AppendQueryParameter(url, "countries", countries);
  }
  if (!fiat_currencies.empty()) {
    url = net::AppendQueryParameter(url, "fiatCurrencies", fiat_currencies);
  }
  if (!crypto_currencies.empty()) {
    url = net::AppendQueryParameter(url, "cryptoCurrencies", crypto_currencies);
  }
  if (!service_providers.empty()) {
    url = net::AppendQueryParameter(url, "serviceProviders", service_providers);
  }
  if (!payment_method_types.empty()) {
    url = net::AppendQueryParameter(url, "paymentMethodTypes",
                                    payment_method_types);
  }

  return url;
}

void BuyAndSellService::GetServiceProviders(
    const std::string& countries,
    const std::string& fiat_currencies,
    const std::string& crypto_currencies,
    const std::string& service_providers,
    const std::string& payment_method_types,
    const std::string& statuses,
    GetServiceProvidersCallback callback) {
  auto internal_callback =
      base::BindOnce(&BuyAndSellService::OnGetServiceProviders,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  api_request_helper_->Request(
      "GET",
      GetServiceProviderURL(countries, fiat_currencies, crypto_currencies,
                            service_providers, payment_method_types, statuses),
      "", "", std::move(internal_callback), MakeMeldApiHeaders(),
      {.auto_retry_on_network_change = true, .enable_cache = true});
}

void BuyAndSellService::OnGetServiceProviders(
    GetServiceProvidersCallback callback,
    APIRequestResult api_request_result) const {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run({},
                            std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
    return;
  }

  if (std::vector<std::string> errors;
      ParseMeldErrorResponse(api_request_result.value_body(), &errors)) {
    std::move(callback).Run({}, errors);
    return;
  }

  std::vector<mojom::ServiceProviderPtr> service_providers;
  if (!ParseServiceProviders(api_request_result.value_body(),
                             &service_providers)) {
    std::move(callback).Run({}, std::vector<std::string>{"PARSING_ERROR"});
    return;
  }
  std::move(callback).Run(std::move(service_providers), std::nullopt);
}

void BuyAndSellService::GetCryptoQuotes(
    const std::string& country,
    const std::string& source_currency_code,
    const std::string& destination_currency_code,
    const double source_amount,
    const std::string& account,
    GetCryptoQuotesCallback callback) {
  base::Value::Dict payload;
  AddKeyIfNotEmpty(&payload, "countryCode", country);
  AddKeyIfNotEmpty(&payload, "sourceCurrencyCode", source_currency_code);
  AddKeyIfNotEmpty(&payload, "sourceAmount",
                   base::NumberToString(source_amount));
  AddKeyIfNotEmpty(&payload, "destinationCurrencyCode",
                   destination_currency_code);
  AddKeyIfNotEmpty(&payload, "walletAddress", account);

  const std::string json_payload = GetJSON(payload);

  auto url = GURL(base::StringPrintf("%s/payments/crypto/quote",
                                     GetMeldAssetRatioBaseURL().c_str()));
  auto internal_callback =
      base::BindOnce(&BuyAndSellService::OnGetCryptoQuotes,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  api_request_helper_->Request(
      "POST", url, json_payload, "application/json",
      std::move(internal_callback), MakeMeldApiHeaders(),
      {.auto_retry_on_network_change = true, .enable_cache = false});
}

void BuyAndSellService::OnGetCryptoQuotes(
    GetCryptoQuotesCallback callback,
    APIRequestResult api_request_result) const {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run({},
                            std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
    return;
  }

  if (std::vector<std::string> errors;
      ParseMeldErrorResponse(api_request_result.value_body(), &errors)) {
    std::move(callback).Run({}, errors);
    return;
  }

  std::optional<std::vector<std::string>> errors;
  std::string error;
  std::vector<mojom::CryptoQuotePtr> quotes;
  if (!ParseCryptoQuotes(api_request_result.value_body(), &quotes, &error)) {
    errors = std::vector<std::string>{"PARSING_ERROR"};
    std::move(callback).Run({}, errors);
    return;
  }

  if (!error.empty()) {
    errors = std::vector<std::string>{error};
  }

  std::move(callback).Run(std::move(quotes), errors);
}

// static
GURL BuyAndSellService::GetGetPaymentMethodsURL(
    const std::string& countries,
    const std::string& fiat_currencies,
    const std::string& crypto_currencies,
    const std::string& service_providers,
    const std::string& payment_method_types,
    const std::string& statuses) {
  auto url =
      GURL(base::StringPrintf("%s/service-providers/properties/payment-methods",
                              GetMeldAssetRatioBaseURL().c_str()));
  url =
      net::AppendQueryParameter(url, "includeServiceProviderDetails", "false");
  if (!statuses.empty()) {
    url = net::AppendQueryParameter(url, "statuses", statuses);
  } else {
    url = net::AppendQueryParameter(url, "statuses", kDefaultMeldStatuses);
  }

  if (!countries.empty()) {
    url = net::AppendQueryParameter(url, "countries", countries);
  }
  if (!fiat_currencies.empty()) {
    url = net::AppendQueryParameter(url, "fiatCurrencies", fiat_currencies);
  }
  if (!crypto_currencies.empty()) {
    url = net::AppendQueryParameter(url, "cryptoCurrencies", crypto_currencies);
  }
  if (!service_providers.empty()) {
    url = net::AppendQueryParameter(url, "serviceProviders", service_providers);
  }
  if (!payment_method_types.empty()) {
    url = net::AppendQueryParameter(url, "paymentMethodTypes",
                                    payment_method_types);
  }

  return url;
}

void BuyAndSellService::GetPaymentMethods(
    const std::string& countries,
    const std::string& fiat_currencies,
    const std::string& crypto_currencies,
    const std::string& service_providers,
    const std::string& payment_method_types,
    const std::string& statuses,
    GetPaymentMethodsCallback callback) {
  auto internal_callback =
      base::BindOnce(&BuyAndSellService::OnGetPaymentMethods,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  api_request_helper_->Request(
      "GET",
      GetGetPaymentMethodsURL(countries, fiat_currencies, crypto_currencies,
                              service_providers, payment_method_types,
                              statuses),
      "", "", std::move(internal_callback), MakeMeldApiHeaders(),
      {.auto_retry_on_network_change = true, .enable_cache = true});
}

void BuyAndSellService::OnGetPaymentMethods(
    GetPaymentMethodsCallback callback,
    APIRequestResult api_request_result) const {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run({},
                            std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
    return;
  }

  if (std::vector<std::string> errors;
      ParseMeldErrorResponse(api_request_result.value_body(), &errors)) {
    std::move(callback).Run({}, errors);
    return;
  }

  std::vector<mojom::PaymentMethodPtr> payment_methods;
  if (!ParsePaymentMethods(api_request_result.value_body(), &payment_methods)) {
    std::move(callback).Run({}, std::vector<std::string>{"PARSING_ERROR"});
    return;
  }

  std::move(callback).Run(std::move(payment_methods), std::nullopt);
}

// static
GURL BuyAndSellService::GetFiatCurrenciesURL(
    const std::string& countries,
    const std::string& fiat_currencies,
    const std::string& crypto_currencies,
    const std::string& service_providers,
    const std::string& payment_method_types,
    const std::string& statuses) {
  auto url =
      GURL(base::StringPrintf("%s/service-providers/properties/fiat-currencies",
                              GetMeldAssetRatioBaseURL().c_str()));
  url =
      net::AppendQueryParameter(url, "includeServiceProviderDetails", "false");
  if (!statuses.empty()) {
    url = net::AppendQueryParameter(url, "statuses", statuses);
  } else {
    url = net::AppendQueryParameter(url, "statuses", kDefaultMeldStatuses);
  }

  if (!countries.empty()) {
    url = net::AppendQueryParameter(url, "countries", countries);
  }
  if (!fiat_currencies.empty()) {
    url = net::AppendQueryParameter(url, "fiatCurrencies", fiat_currencies);
  }
  if (!crypto_currencies.empty()) {
    url = net::AppendQueryParameter(url, "cryptoCurrencies", crypto_currencies);
  }
  if (!service_providers.empty()) {
    url = net::AppendQueryParameter(url, "serviceProviders", service_providers);
  }
  if (!payment_method_types.empty()) {
    url = net::AppendQueryParameter(url, "paymentMethodTypes",
                                    payment_method_types);
  }

  return url;
}

void BuyAndSellService::GetFiatCurrencies(
    const std::string& countries,
    const std::string& fiat_currencies,
    const std::string& crypto_currencies,
    const std::string& service_providers,
    const std::string& payment_method_types,
    const std::string& statuses,
    GetFiatCurrenciesCallback callback) {
  auto internal_callback =
      base::BindOnce(&BuyAndSellService::OnGetFiatCurrencies,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  api_request_helper_->Request(
      "GET",
      GetFiatCurrenciesURL(countries, fiat_currencies, crypto_currencies,
                           service_providers, payment_method_types, statuses),
      "", "", std::move(internal_callback), MakeMeldApiHeaders(),
      {.auto_retry_on_network_change = true, .enable_cache = true});
}

void BuyAndSellService::OnGetFiatCurrencies(
    GetFiatCurrenciesCallback callback,
    APIRequestResult api_request_result) const {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run({},
                            std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
    return;
  }

  if (std::vector<std::string> errors;
      ParseMeldErrorResponse(api_request_result.value_body(), &errors)) {
    std::move(callback).Run({}, errors);
    return;
  }

  std::vector<mojom::FiatCurrencyPtr> fiat_currencies;
  if (!ParseFiatCurrencies(api_request_result.value_body(), &fiat_currencies)) {
    std::move(callback).Run({}, std::vector<std::string>{"PARSING_ERROR"});
    return;
  }

  std::move(callback).Run(std::move(fiat_currencies), std::nullopt);
}

// static
GURL BuyAndSellService::GetCryptoCurrenciesURL(
    const std::string& countries,
    const std::string& fiat_currencies,
    const std::string& crypto_currencies,
    const std::string& service_providers,
    const std::string& payment_method_types,
    const std::string& statuses) {
  auto url = GURL(
      base::StringPrintf("%s/service-providers/properties/crypto-currencies",
                         GetMeldAssetRatioBaseURL().c_str()));

  url =
      net::AppendQueryParameter(url, "includeServiceProviderDetails", "false");

  if (!statuses.empty()) {
    url = net::AppendQueryParameter(url, "statuses", statuses);
  } else {
    url = net::AppendQueryParameter(url, "statuses", kDefaultMeldStatuses);
  }

  if (!countries.empty()) {
    url = net::AppendQueryParameter(url, "countries", countries);
  }
  if (!fiat_currencies.empty()) {
    url = net::AppendQueryParameter(url, "fiatCurrencies", fiat_currencies);
  }
  if (!crypto_currencies.empty()) {
    url = net::AppendQueryParameter(url, "cryptoCurrencies", crypto_currencies);
  }
  if (!service_providers.empty()) {
    url = net::AppendQueryParameter(url, "serviceProviders", service_providers);
  }
  if (!payment_method_types.empty()) {
    url = net::AppendQueryParameter(url, "paymentMethodTypes",
                                    payment_method_types);
  }

  return url;
}

void BuyAndSellService::GetCryptoCurrencies(
    const std::string& countries,
    const std::string& fiat_currencies,
    const std::string& crypto_currencies,
    const std::string& service_providers,
    const std::string& payment_method_types,
    const std::string& statuses,
    GetCryptoCurrenciesCallback callback) {
  auto internal_callback =
      base::BindOnce(&BuyAndSellService::OnGetCryptoCurrencies,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  api_request_helper_->Request(
      "GET",
      GetCryptoCurrenciesURL(countries, fiat_currencies, crypto_currencies,
                             service_providers, payment_method_types, statuses),
      "", "", std::move(internal_callback), MakeMeldApiHeaders(),
      {.auto_retry_on_network_change = true, .enable_cache = true});
}

void BuyAndSellService::OnGetCryptoCurrencies(
    GetCryptoCurrenciesCallback callback,
    APIRequestResult api_request_result) const {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run({},
                            std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
    return;
  }

  if (std::vector<std::string> errors;
      ParseMeldErrorResponse(api_request_result.value_body(), &errors)) {
    std::move(callback).Run({}, errors);
    return;
  }

  std::vector<mojom::CryptoCurrencyPtr> crypto_currencies;
  if (!ParseCryptoCurrencies(api_request_result.value_body(),
                             &crypto_currencies)) {
    std::move(callback).Run({}, std::vector<std::string>{"PARSING_ERROR"});
    return;
  }

  std::move(callback).Run(std::move(crypto_currencies), std::nullopt);
}

// static
GURL BuyAndSellService::GetCountriesURL(const std::string& countries,
                                        const std::string& fiat_currencies,
                                        const std::string& crypto_currencies,
                                        const std::string& service_providers,
                                        const std::string& payment_method_types,
                                        const std::string& statuses) {
  auto url =
      GURL(base::StringPrintf("%s/service-providers/properties/countries",
                              GetMeldAssetRatioBaseURL().c_str()));

  url =
      net::AppendQueryParameter(url, "includeServiceProviderDetails", "false");

  if (!statuses.empty()) {
    url = net::AppendQueryParameter(url, "statuses", statuses);
  } else {
    url = net::AppendQueryParameter(url, "statuses", kDefaultMeldStatuses);
  }

  if (!countries.empty()) {
    url = net::AppendQueryParameter(url, "countries", countries);
  }
  if (!fiat_currencies.empty()) {
    url = net::AppendQueryParameter(url, "fiatCurrencies", fiat_currencies);
  }
  if (!crypto_currencies.empty()) {
    url = net::AppendQueryParameter(url, "cryptoCurrencies", crypto_currencies);
  }
  if (!service_providers.empty()) {
    url = net::AppendQueryParameter(url, "serviceProviders", service_providers);
  }
  if (!payment_method_types.empty()) {
    url = net::AppendQueryParameter(url, "paymentMethodTypes",
                                    payment_method_types);
  }

  return url;
}

void BuyAndSellService::GetCountries(const std::string& countries,
                                     const std::string& fiat_currencies,
                                     const std::string& crypto_currencies,
                                     const std::string& service_providers,
                                     const std::string& payment_method_types,
                                     const std::string& statuses,
                                     GetCountriesCallback callback) {
  auto internal_callback =
      base::BindOnce(&BuyAndSellService::OnGetCountries,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  api_request_helper_->Request(
      "GET",
      GetCountriesURL(countries, fiat_currencies, crypto_currencies,
                      service_providers, payment_method_types, statuses),
      "", "", std::move(internal_callback), MakeMeldApiHeaders(),
      {.auto_retry_on_network_change = true, .enable_cache = true});
}

void BuyAndSellService::OnGetCountries(
    GetCountriesCallback callback,
    APIRequestResult api_request_result) const {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run({},
                            std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
    return;
  }

  if (std::vector<std::string> errors;
      ParseMeldErrorResponse(api_request_result.value_body(), &errors)) {
    std::move(callback).Run({}, errors);
    return;
  }

  std::vector<mojom::CountryPtr> countries;
  if (!ParseCountries(api_request_result.value_body(), &countries)) {
    std::move(callback).Run({}, std::vector<std::string>{"PARSING_ERROR"});
    return;
  }

  std::move(callback).Run(std::move(countries), std::nullopt);
}

}  // namespace brave_wallet
