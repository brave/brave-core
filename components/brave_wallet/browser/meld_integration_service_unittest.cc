/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/meld_integration_service.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/meld_integration.mojom-forward.h"
#include "brave/components/brave_wallet/common/meld_integration.mojom.h"
#include "components/grit/brave_components_strings.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/test/test_url_loader_factory.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

void CheckOptString(const std::string* val,
                    const std::optional<std::string>& val_to_check) {
  if (val_to_check) {
    EXPECT_TRUE(val);
    EXPECT_EQ(val_to_check, *val);
  } else {
    EXPECT_FALSE(val);
  }
}
void CheckString(const std::string* val, const std::string& val_to_check) {
  EXPECT_TRUE(val);
  EXPECT_EQ(val_to_check, *val);
}
}  // namespace

namespace brave_wallet {

class MeldIntegrationServiceUnitTest : public testing::Test {
 public:
  MeldIntegrationServiceUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {
    meld_integration_service_ =
        std::make_unique<MeldIntegrationService>(shared_url_loader_factory_);
  }

  ~MeldIntegrationServiceUnitTest() override = default;

  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory() {
    return shared_url_loader_factory_;
  }

  using OnRequestPayloadCallback =
      base::RepeatingCallback<void(const std::string& request_payload)>;

  void SetInterceptor(
      const std::string& content,
      const net::HttpStatusCode http_status,
      OnRequestPayloadCallback* request_payload_callback = nullptr) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, content, http_status,
         request_payload_callback](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          std::string header;
          EXPECT_TRUE(request.headers.GetHeader(
              brave_wallet::kMeldRpcVersionHeader, &header));
          EXPECT_FALSE(header.empty());

          if (request_payload_callback) {
            std::string request_string(request.request_body->elements()
                                           ->at(0)
                                           .As<network::DataElementBytes>()
                                           .AsStringPiece());
            request_payload_callback->Run(request_string);
          }
          url_loader_factory_.AddResponse(request.url.spec(), content,
                                          http_status);
        }));
  }

  void TestGetServiceProvider(
      const std::string& content,
      const std::string& countries,
      const std::string& fiat_currencies,
      const std::string& crypto_currencies,
      const std::string& crypto_chains,
      const std::string& service_providers,
      const std::string& payment_method_types,
      const std::string& statuses,
      MeldIntegrationService::GetServiceProvidersCallback callback,
      const net::HttpStatusCode http_status = net::HTTP_OK) {
    SetInterceptor(content, http_status);

    base::RunLoop run_loop;
    base::MockCallback<MeldIntegrationService::GetServiceProvidersCallback>
        mock_callback;
    EXPECT_CALL(mock_callback, Run)
        .Times(1)
        .WillRepeatedly(
            [&](std::optional<std::vector<mojom::MeldServiceProviderPtr>> sps,
                const std::optional<std::vector<std::string>>& errors) {
              std::move(callback).Run(std::move(sps), errors);
              run_loop.Quit();
            });
    auto filter = mojom::MeldFilter::New(
        countries, fiat_currencies, crypto_currencies, crypto_chains,
        service_providers, payment_method_types, statuses);

    meld_integration_service_->GetServiceProviders(std::move(filter),
                                                   mock_callback.Get());
    run_loop.Run();
  }

  void TestGetCryptoQuotes(
      const std::string& content,
      const std::string& country,
      const std::string& from_asset,
      const std::string& to_asset,
      const double source_amount,
      const std::string& account,
      MeldIntegrationService::GetCryptoQuotesCallback callback,
      const net::HttpStatusCode http_status = net::HTTP_OK) {
    SetInterceptor(content, http_status);

    base::RunLoop run_loop;
    base::MockCallback<MeldIntegrationService::GetCryptoQuotesCallback>
        mock_callback;
    EXPECT_CALL(mock_callback, Run)
        .Times(1)
        .WillRepeatedly(
            [&](std::optional<std::vector<mojom::MeldCryptoQuotePtr>> quotes,
                const std::optional<std::vector<std::string>>& errors) {
              std::move(callback).Run(std::move(quotes), errors);
              run_loop.Quit();
            });

    meld_integration_service_->GetCryptoQuotes(country, from_asset, to_asset,
                                               source_amount, account,
                                               mock_callback.Get());
    run_loop.Run();
  }

  void TestGetPaymentMethods(
      const std::string& content,
      const std::string& countries,
      const std::string& fiat_currencies,
      const std::string& crypto_currencies,
      const std::string& crypto_chains,
      const std::string& service_providers,
      const std::string& payment_method_types,
      const std::string& statuses,
      MeldIntegrationService::GetPaymentMethodsCallback callback,
      const net::HttpStatusCode http_status = net::HTTP_OK) {
    SetInterceptor(content, http_status);

    base::RunLoop run_loop;
    base::MockCallback<MeldIntegrationService::GetPaymentMethodsCallback>
        mock_callback;
    EXPECT_CALL(mock_callback, Run)
        .Times(1)
        .WillRepeatedly(
            [&](std::optional<std::vector<mojom::MeldPaymentMethodPtr>>
                    payment_methods,
                const std::optional<std::vector<std::string>>& errors) {
              std::move(callback).Run(std::move(payment_methods), errors);
              run_loop.Quit();
            });

    auto filter = mojom::MeldFilter::New(
        countries, fiat_currencies, crypto_currencies, crypto_chains,
        service_providers, payment_method_types, statuses);

    meld_integration_service_->GetPaymentMethods(std::move(filter),
                                                 mock_callback.Get());
    run_loop.Run();
  }

  void TestGetFiatCurrencies(
      const std::string& content,
      const std::string& countries,
      const std::string& fiat_currencies,
      const std::string& crypto_currencies,
      const std::string& crypto_chains,
      const std::string& service_providers,
      const std::string& payment_method_types,
      const std::string& statuses,
      MeldIntegrationService::GetFiatCurrenciesCallback callback,
      const net::HttpStatusCode http_status = net::HTTP_OK) {
    SetInterceptor(content, http_status);

    base::RunLoop run_loop;
    base::MockCallback<MeldIntegrationService::GetFiatCurrenciesCallback>
        mock_callback;
    EXPECT_CALL(mock_callback, Run)
        .Times(1)
        .WillRepeatedly(
            [&](std::optional<std::vector<mojom::MeldFiatCurrencyPtr>>
                    fiat_currencies,
                const std::optional<std::vector<std::string>>& errors) {
              std::move(callback).Run(std::move(fiat_currencies), errors);
              run_loop.Quit();
            });
    auto filter = mojom::MeldFilter::New(
        countries, fiat_currencies, crypto_currencies, crypto_chains,
        service_providers, payment_method_types, statuses);

    meld_integration_service_->GetFiatCurrencies(std::move(filter),
                                                 mock_callback.Get());
    run_loop.Run();
  }

  void TestGetCryptoCurrencies(
      const std::string& content,
      const std::string& countries,
      const std::string& fiat_currencies,
      const std::string& crypto_currencies,
      const std::string& crypto_chains,
      const std::string& service_providers,
      const std::string& payment_method_types,
      const std::string& statuses,
      MeldIntegrationService::GetCryptoCurrenciesCallback callback,
      const net::HttpStatusCode http_status = net::HTTP_OK) {
    SetInterceptor(content, http_status);

    base::RunLoop run_loop;
    base::MockCallback<MeldIntegrationService::GetCryptoCurrenciesCallback>
        mock_callback;
    EXPECT_CALL(mock_callback, Run)
        .Times(1)
        .WillRepeatedly(
            [&](std::optional<std::vector<mojom::MeldCryptoCurrencyPtr>>
                    crypto_currencies,
                const std::optional<std::vector<std::string>>& errors) {
              std::move(callback).Run(std::move(crypto_currencies), errors);
              run_loop.Quit();
            });
    auto filter = mojom::MeldFilter::New(
        countries, fiat_currencies, crypto_currencies, crypto_chains,
        service_providers, payment_method_types, statuses);
    meld_integration_service_->GetCryptoCurrencies(std::move(filter),
                                                   mock_callback.Get());
    run_loop.Run();
  }

  void TestGetCountries(const std::string& content,
                        const std::string& countries,
                        const std::string& fiat_currencies,
                        const std::string& crypto_currencies,
                        const std::string& crypto_chains,
                        const std::string& service_providers,
                        const std::string& payment_method_types,
                        const std::string& statuses,
                        MeldIntegrationService::GetCountriesCallback callback,
                        const net::HttpStatusCode http_status = net::HTTP_OK) {
    SetInterceptor(content, http_status);

    base::RunLoop run_loop;
    base::MockCallback<MeldIntegrationService::GetCountriesCallback>
        mock_callback;
    EXPECT_CALL(mock_callback, Run)
        .Times(1)
        .WillRepeatedly(
            [&](std::optional<std::vector<mojom::MeldCountryPtr>> countries,
                const std::optional<std::vector<std::string>>& errors) {
              std::move(callback).Run(std::move(countries), errors);
              run_loop.Quit();
            });
    auto filter = mojom::MeldFilter::New(
        countries, fiat_currencies, crypto_currencies, crypto_chains,
        service_providers, payment_method_types, statuses);
    meld_integration_service_->GetCountries(std::move(filter),
                                            mock_callback.Get());
    run_loop.Run();
  }

  void TestCryptoBuyWidgetCreate(
      const std::string& content,
      const std::string& country_code,
      const std::string& destination_currency_code,
      std::optional<std::vector<std::string>> lock_fields,
      const std::optional<std::string>& payment_method_type,
      const std::optional<std::string>& redirect_url,
      const std::string& service_provider,
      const std::string& source_amount,
      const std::string& source_currency_code,
      const std::string& wallet_address,
      const std::optional<std::string>& wallet_tag,
      const std::optional<std::string>& customer_object_email,
      const std::optional<std::string>& customer_id,
      const std::optional<std::string>& external_customer_id,
      const std::optional<std::string>& external_session_id,
      MeldIntegrationService::CryptoBuyWidgetCreateCallback callback,
      const net::HttpStatusCode http_status = net::HTTP_OK,
      const bool is_session_data_null = false) {
    mojom::CryptoBuySessionDataPtr session_data;
    if (!is_session_data_null) {
      session_data = mojom::CryptoBuySessionData::New(
          country_code, destination_currency_code, lock_fields,
          payment_method_type, redirect_url, service_provider, source_amount,
          source_currency_code, wallet_address, wallet_tag);
    }

    mojom::CryptoWidgetCustomerDataPtr customer_data;
    if (customer_object_email || customer_id || external_customer_id ||
        external_session_id) {
      customer_data = mojom::CryptoWidgetCustomerData::New(
          customer_object_email
              ? mojom::CustomerObject::New(customer_object_email.value())
              : nullptr,
          customer_id, external_customer_id, external_session_id);
    }

    auto request_payload_check_callback = base::BindRepeating(
        [](const std::optional<std::string>& customer_id,
           const std::optional<std::string>& external_customer_id,
           const std::optional<std::string>& external_session_id,
           const std::string& country_code,
           const std::string& destination_currency_code,
           const std::optional<std::string>& payment_method_type,
           const std::optional<std::string>& redirect_url,
           const std::string& service_provider,
           const std::string& source_amount,
           const std::string& source_currency_code,
           const std::string& wallet_address,
           const std::optional<std::string>& wallet_tag,
           const std::string& request_payload) {
          auto request_payload_value = base::test::ParseJson(request_payload);
          EXPECT_TRUE(request_payload_value.is_dict());
          auto& request_payload_dict = request_payload_value.GetDict();

          CheckString(request_payload_dict.FindString("sessionType"), "BUY");

          CheckOptString(request_payload_dict.FindString("customerId"),
                         customer_id);
          CheckOptString(request_payload_dict.FindString("externalCustomerId"),
                         external_customer_id);
          CheckOptString(request_payload_dict.FindString("externalSessionId"),
                         external_session_id);

          auto* session_data_dict =
              request_payload_dict.FindDict("sessionData");
          EXPECT_TRUE(session_data_dict);
          CheckString(session_data_dict->FindString("countryCode"),
                      country_code);
          CheckString(session_data_dict->FindString("destinationCurrencyCode"),
                      destination_currency_code);
          CheckOptString(session_data_dict->FindString("paymentMethodType"),
                         payment_method_type);
          CheckOptString(session_data_dict->FindString("redirectUrl"),
                         redirect_url);
          CheckString(session_data_dict->FindString("serviceProvider"),
                      service_provider);
          CheckString(session_data_dict->FindString("sourceAmount"),
                      source_amount);
          CheckString(session_data_dict->FindString("sourceCurrencyCode"),
                      source_currency_code);
          CheckString(session_data_dict->FindString("walletAddress"),
                      wallet_address);
          CheckOptString(session_data_dict->FindString("walletTag"),
                         wallet_tag);
        },
        customer_id, external_customer_id, external_session_id, country_code,
        destination_currency_code, payment_method_type, redirect_url,
        service_provider, source_amount, source_currency_code, wallet_address,
        wallet_tag);

    SetInterceptor(content, http_status, &request_payload_check_callback);

    base::RunLoop run_loop;
    base::MockCallback<MeldIntegrationService::CryptoBuyWidgetCreateCallback>
        mock_callback;
    EXPECT_CALL(mock_callback, Run)
        .Times(1)
        .WillRepeatedly(
            [&](mojom::MeldCryptoWidgetPtr crypto_widget,
                const std::optional<std::vector<std::string>>& errors) {
              std::move(callback).Run(std::move(crypto_widget), errors);
              run_loop.Quit();
            });
    meld_integration_service_->CryptoBuyWidgetCreate(
        std::move(session_data), std::move(customer_data), mock_callback.Get());
    run_loop.Run();
  }

  void TestCryptoSellWidgetCreate(
      const std::string& content,
      const std::string& country_code,
      const std::string& destination_currency_code,
      std::optional<std::vector<std::string>> lock_fields,
      const std::optional<std::string>& payment_method_type,
      const std::optional<std::string>& redirect_url,
      const std::string& service_provider,
      const std::string& source_amount,
      const std::string& source_currency_code,
      const std::optional<std::string>& wallet_address,
      const std::optional<std::string>& wallet_tag,
      const std::optional<std::string>& customer_object_email,
      const std::optional<std::string>& customer_id,
      const std::optional<std::string>& external_customer_id,
      const std::optional<std::string>& external_session_id,
      MeldIntegrationService::CryptoBuyWidgetCreateCallback callback,
      const net::HttpStatusCode http_status = net::HTTP_OK,
      const bool is_session_data_null = false) {
    mojom::CryptoSellSessionDataPtr session_data;
    if (!is_session_data_null) {
      session_data = mojom::CryptoSellSessionData::New(
          country_code, destination_currency_code, lock_fields,
          payment_method_type, redirect_url, service_provider, source_amount,
          source_currency_code, wallet_address, wallet_tag);
    }

    mojom::CryptoWidgetCustomerDataPtr customer_data;
    if (customer_object_email || customer_id || external_customer_id ||
        external_session_id) {
      customer_data = mojom::CryptoWidgetCustomerData::New(
          customer_object_email
              ? mojom::CustomerObject::New(customer_object_email.value())
              : nullptr,
          customer_id, external_customer_id, external_session_id);
    }

    auto request_payload_check_callback = base::BindRepeating(
        [](const std::optional<std::string>& customer_id,
           const std::optional<std::string>& external_customer_id,
           const std::optional<std::string>& external_session_id,
           const std::string& country_code,
           const std::string& destination_currency_code,
           const std::optional<std::string>& payment_method_type,
           const std::optional<std::string>& redirect_url,
           const std::string& service_provider,
           const std::string& source_amount,
           const std::string& source_currency_code,
           const std::optional<std::string>& wallet_address,
           const std::optional<std::string>& wallet_tag,
           const std::string& request_payload) {
          auto request_payload_value = base::test::ParseJson(request_payload);
          EXPECT_TRUE(request_payload_value.is_dict());
          auto& request_payload_dict = request_payload_value.GetDict();

          CheckString(request_payload_dict.FindString("sessionType"), "SELL");

          CheckOptString(request_payload_dict.FindString("customerId"),
                         customer_id);
          CheckOptString(request_payload_dict.FindString("externalCustomerId"),
                         external_customer_id);
          CheckOptString(request_payload_dict.FindString("externalSessionId"),
                         external_session_id);

          auto* session_data_dict =
              request_payload_dict.FindDict("sessionData");
          EXPECT_TRUE(session_data_dict);
          CheckString(session_data_dict->FindString("countryCode"),
                      country_code);
          CheckString(session_data_dict->FindString("destinationCurrencyCode"),
                      destination_currency_code);
          CheckOptString(session_data_dict->FindString("paymentMethodType"),
                         payment_method_type);
          CheckOptString(session_data_dict->FindString("redirectUrl"),
                         redirect_url);
          CheckString(session_data_dict->FindString("serviceProvider"),
                      service_provider);
          CheckString(session_data_dict->FindString("sourceAmount"),
                      source_amount);
          CheckString(session_data_dict->FindString("sourceCurrencyCode"),
                      source_currency_code);
          CheckOptString(session_data_dict->FindString("walletAddress"),
                         wallet_address);
          CheckOptString(session_data_dict->FindString("walletTag"),
                         wallet_tag);
        },
        customer_id, external_customer_id, external_session_id, country_code,
        destination_currency_code, payment_method_type, redirect_url,
        service_provider, source_amount, source_currency_code, wallet_address,
        wallet_tag);

    SetInterceptor(content, http_status, &request_payload_check_callback);

    base::RunLoop run_loop;
    base::MockCallback<MeldIntegrationService::CryptoBuyWidgetCreateCallback>
        mock_callback;
    EXPECT_CALL(mock_callback, Run)
        .Times(1)
        .WillRepeatedly(
            [&](mojom::MeldCryptoWidgetPtr crypto_widget,
                const std::optional<std::vector<std::string>>& errors) {
              std::move(callback).Run(std::move(crypto_widget), errors);
              run_loop.Quit();
            });
    meld_integration_service_->CryptoSellWidgetCreate(
        std::move(session_data), std::move(customer_data), mock_callback.Get());
    run_loop.Run();
  }

  void TestCryptoTransferWidgetCreate(
      const std::string& content,
      const std::optional<std::string>& country_code,
      const std::optional<std::string>& institution_id,
      std::optional<std::vector<std::string>> lock_fields,
      const std::optional<std::string>& redirect_url,
      const std::string& service_provider,
      const std::optional<std::string>& source_amount,
      const std::vector<std::string>& source_currency_codes,
      const std::optional<std::string>& wallet_address,
      const std::optional<std::string>& wallet_tag,
      const std::optional<std::string>& customer_object_email,
      const std::optional<std::string>& customer_id,
      const std::optional<std::string>& external_customer_id,
      const std::optional<std::string>& external_session_id,
      MeldIntegrationService::CryptoBuyWidgetCreateCallback callback,
      const net::HttpStatusCode http_status = net::HTTP_OK,
      const bool is_session_data_null = false) {
    mojom::CryptoTransferSessionDataPtr session_data;
    if (!is_session_data_null) {
      session_data = mojom::CryptoTransferSessionData::New(
          country_code, institution_id, lock_fields, redirect_url,
          service_provider, source_amount, source_currency_codes,
          wallet_address, wallet_tag);
    }

    mojom::CryptoWidgetCustomerDataPtr customer_data;
    if (customer_object_email || customer_id || external_customer_id ||
        external_session_id) {
      customer_data = mojom::CryptoWidgetCustomerData::New(
          customer_object_email
              ? mojom::CustomerObject::New(customer_object_email.value())
              : nullptr,
          customer_id, external_customer_id, external_session_id);
    }

    auto request_payload_check_callback = base::BindRepeating(
        [](const std::optional<std::string>& customer_id,
           const std::optional<std::string>& external_customer_id,
           const std::optional<std::string>& external_session_id,
           const std::optional<std::string>& country_code,
           const std::optional<std::string>& institution_id,
           const std::optional<std::string>& redirect_url,
           const std::string& service_provider,
           const std::optional<std::string>& source_amount,
           const std::vector<std::string>& source_currency_codes,
           const std::optional<std::string>& wallet_address,
           const std::optional<std::string>& wallet_tag,
           const std::string& request_payload) {
          auto request_payload_value = base::test::ParseJson(request_payload);
          EXPECT_TRUE(request_payload_value.is_dict());
          auto& request_payload_dict = request_payload_value.GetDict();

          CheckString(request_payload_dict.FindString("sessionType"),
                      "TRANSFER");

          CheckOptString(request_payload_dict.FindString("customerId"),
                         customer_id);
          CheckOptString(request_payload_dict.FindString("externalCustomerId"),
                         external_customer_id);
          CheckOptString(request_payload_dict.FindString("externalSessionId"),
                         external_session_id);

          auto* session_data_dict =
              request_payload_dict.FindDict("sessionData");
          EXPECT_TRUE(session_data_dict);
          CheckOptString(session_data_dict->FindString("countryCode"),
                         country_code);
          CheckOptString(session_data_dict->FindString("institutionId"),
                         institution_id);
          CheckOptString(session_data_dict->FindString("redirectUrl"),
                         redirect_url);
          CheckString(session_data_dict->FindString("serviceProvider"),
                      service_provider);
          CheckOptString(session_data_dict->FindString("sourceAmount"),
                         source_amount);

          auto* source_currency_codes_list =
              session_data_dict->FindList("sourceCurrencyCodes");
          EXPECT_TRUE(source_currency_codes_list);
          for (const auto& item_value : *source_currency_codes_list) {
            EXPECT_EQ(base::ranges::count_if(
                          source_currency_codes,
                          [&](const auto& item) { return item == item_value; }),
                      1);
          }
          CheckOptString(session_data_dict->FindString("walletAddress"),
                         wallet_address);
          CheckOptString(session_data_dict->FindString("walletTag"),
                         wallet_tag);
        },
        customer_id, external_customer_id, external_session_id, country_code,
        institution_id, redirect_url, service_provider, source_amount,
        source_currency_codes, wallet_address, wallet_tag);

    SetInterceptor(content, http_status, &request_payload_check_callback);

    base::RunLoop run_loop;
    base::MockCallback<MeldIntegrationService::CryptoBuyWidgetCreateCallback>
        mock_callback;
    EXPECT_CALL(mock_callback, Run)
        .Times(1)
        .WillRepeatedly(
            [&](mojom::MeldCryptoWidgetPtr crypto_widget,
                const std::optional<std::vector<std::string>>& errors) {
              std::move(callback).Run(std::move(crypto_widget), errors);
              run_loop.Quit();
            });
    meld_integration_service_->CryptoTransferWidgetCreate(
        std::move(session_data), std::move(customer_data), mock_callback.Get());
    run_loop.Run();
  }

 protected:
  std::unique_ptr<MeldIntegrationService> meld_integration_service_;
  base::test::TaskEnvironment task_environment_;

 private:
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
};

TEST_F(MeldIntegrationServiceUnitTest, GetServiceProviders) {
  auto filter = mojom::MeldFilter::New(
      "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", std::nullopt);
  const auto url = MeldIntegrationService::GetServiceProviderURL(filter);
  EXPECT_EQ(url.path(), "/service-providers");
  EXPECT_EQ(url.query(),
            "accountFilter=false&statuses=LIVE%2CRECENTLY_ADDED&countries=US%"
            "2CCA&fiatCurrencies=USD%2CEUR&cryptoCurrencies=BTC%2CETH&"
            "cryptoChains=BTC%2CDOGE&"
            "serviceProviders=BANXA%2CBLOCKCHAINDOTCOM&paymentMethodTypes="
            "MOBILE_WALLET%2CBANK_TRANSFER");
  TestGetServiceProvider(
      R"([
  {
    "serviceProvider": "BANXA",
    "name": "Banxa",
    "status": "LIVE",
    "categories": [
      "CRYPTO_ONRAMP"
    ],
    "categoryStatuses": {
      "CRYPTO_ONRAMP": "LIVE"
    },
    "websiteUrl": "http://www.banxa.com",
    "logos": {
      "dark": "https://images-serviceprovider.meld.io/BANXA/logo_dark.png",
      "light": "https://images-serviceprovider.meld.io/BANXA/logo_light.png",
      "darkShort": "https://images-serviceprovider.meld.io/BANXA/short_logo_dark.png",
      "lightShort": null
    }
  },
  {
    "serviceProvider": "BLOCKCHAINDOTCOM",
    "name": "Blockchain.com",
    "status": "LIVE",
    "categories": [
      "CRYPTO_ONRAMP"
    ],
    "categoryStatuses": {
      "CRYPTO_ONRAMP": "LIVE"
    },
    "websiteUrl": "https://www.blockchain.com",
    "logos": null
  }])",
      "US", "USD", "ETH", "BTC,DOGE", "", "", "",
      base::BindLambdaForTesting(
          [&](std::optional<std::vector<mojom::MeldServiceProviderPtr>> sps,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_FALSE(errors.has_value());
            EXPECT_EQ(base::ranges::count_if(
                          *sps,
                          [](const auto& item) {
                            return item->name == "Banxa" &&
                                   item->service_provider == "BANXA" &&
                                   item->status == "LIVE" &&
                                   !item->categories.empty() &&
                                   item->categories[0] == "CRYPTO_ONRAMP" &&
                                   item->web_site_url ==
                                       "http://www.banxa.com" &&
                                   item->logo_images &&
                                   item->logo_images->dark_url ==
                                       "https://images-serviceprovider.meld.io/"
                                       "BANXA/logo_dark.png" &&
                                   item->logo_images->dark_short_url ==
                                       "https://images-serviceprovider.meld.io/"
                                       "BANXA/short_logo_dark.png" &&
                                   item->logo_images->light_url ==
                                       "https://images-serviceprovider.meld.io/"
                                       "BANXA/logo_light.png" &&
                                   !item->logo_images->light_short_url;
                          }),
                      1);
            EXPECT_EQ(base::ranges::count_if(
                          *sps,
                          [](const auto& item) {
                            return item->name == "Blockchain.com" &&
                                   item->service_provider ==
                                       "BLOCKCHAINDOTCOM" &&
                                   item->status == "LIVE" &&
                                   item->categories[0] == "CRYPTO_ONRAMP" &&
                                   item->web_site_url ==
                                       "https://www.blockchain.com" &&
                                   !item->logo_images;
                          }),
                      1);
          }));
  TestGetServiceProvider(
      R"([{
    "status": "LIVE",
    "categories": [
      "CRYPTO_ONRAMP"
    ],
    "categoryStatuses": {
      "CRYPTO_ONRAMP": "LIVE"
    }
  }])",
      "US", "USD", "ETH", "BTC,DOGE", "", "", "",
      base::BindLambdaForTesting(
          [&](std::optional<std::vector<mojom::MeldServiceProviderPtr>> sps,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});
          }));
  TestGetServiceProvider(
      R"({
    "code": "UNAUTHORIZED",
    "message": "invalid profile or secret",
    "requestId": "315a",
    "timestamp": "2024-04-24T18:55:09.327818Z"
  })",
      "US", "USD", "ETH", "BTC,DOGE", "", "", "",
      base::BindLambdaForTesting(
          [&](std::optional<std::vector<mojom::MeldServiceProviderPtr>> sps,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{"invalid profile or secret"});
          }),
      net::HTTP_UNAUTHORIZED);
  TestGetServiceProvider(
      "some wrone data", "US", "USD", "ETH", "BTC,DOGE", "", "", "",
      base::BindLambdaForTesting(
          [&](std::optional<std::vector<mojom::MeldServiceProviderPtr>> sps,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
          }));
  TestGetServiceProvider(
      "some wrone data", "US", "USD", "ETH", "BTC,DOGE", "", "", "",
      base::BindLambdaForTesting(
          [&](std::optional<std::vector<mojom::MeldServiceProviderPtr>> sps,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
          }),
      net::HTTP_REQUEST_TIMEOUT);
  TestGetServiceProvider(
      R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [
      "[sourceAmount] must not be null",
      "[sourceCurrencyCode] must not be blank"
    ],
    "requestId": "356d",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })",
      "US", "USD", "ETH", "BTC,DOGE", "", "", "",
      base::BindLambdaForTesting(
          [&](std::optional<std::vector<mojom::MeldServiceProviderPtr>> sps,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>(
                                   {"[sourceAmount] must not be null",
                                    "[sourceCurrencyCode] must not be blank"}));
          }),
      net::HTTP_BAD_REQUEST);
}

TEST_F(MeldIntegrationServiceUnitTest, GetCryptoQuotes) {
  TestGetCryptoQuotes(
      R"({
    "quotes": [
      {
        "transactionType": "CRYPTO_PURCHASE",
        "sourceAmount": 50,
        "sourceAmountWithoutFees": 43.97,
        "fiatAmountWithoutFees": 43.97,
        "destinationAmountWithoutFees": null,
        "sourceCurrencyCode": "USD",
        "countryCode": "US",
        "totalFee": 6.03,
        "networkFee": 3.53,
        "transactionFee": 2,
        "destinationAmount": 0.00066413,
        "destinationCurrencyCode": "BTC",
        "exchangeRate": 75286,
        "paymentMethodType": "APPLE_PAY",
        "customerScore": 20,
        "serviceProvider": "TRANSAK"
      }
    ],
    "message": null,
    "error": null
  })",
      "US", "USD", "BTC", 50, "btc account address",
      base::BindLambdaForTesting(
          [](std::optional<std::vector<mojom::MeldCryptoQuotePtr>> quotes,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_FALSE(errors.has_value());
            EXPECT_EQ(base::ranges::count_if(
                          *quotes,
                          [](const auto& item) {
                            return item->transaction_type ==
                                       "CRYPTO_PURCHASE" &&
                                   item->source_amount == "50" &&
                                   item->source_amount_without_fee == "43.97" &&
                                   item->fiat_amount_without_fees == "43.97" &&
                                   item->destination_amount_without_fees ==
                                       std::nullopt &&
                                   item->source_currency_code == "USD" &&
                                   item->country_code == "US" &&
                                   item->total_fee == "6.03" &&
                                   item->network_fee == "3.53" &&
                                   item->transaction_fee == "2" &&
                                   item->destination_amount == "0.00066413" &&
                                   item->destination_currency_code == "BTC" &&
                                   item->exchange_rate == "75286" &&
                                   item->payment_method == "APPLE_PAY" &&
                                   item->customer_score == "20" &&
                                   item->service_provider == "TRANSAK";
                          }),
                      1);
          }));
  TestGetCryptoQuotes(
      "some wrong data", "US", "USD", "BTC", 50, "btc account address",
      base::BindLambdaForTesting(
          [](std::optional<std::vector<mojom::MeldCryptoQuotePtr>> quotes,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
          }),
      net::HTTP_REQUEST_TIMEOUT);

  TestGetCryptoQuotes(
      R"({
    "quotes": [
      {
        "transactionType": "CRYPTO_PURCHASE",
        "sourceAmount": 50,
        "sourceAmountWithoutFees": 43.97,
        "fiatAmountWithoutFees": 43.97,
        "destinationAmountWithoutFees": null,
        "sourceCurrencyCode": "USD",
        "countryCode": "US",
        "totalFee": null,
        "networkFee": 3.53,
        "transactionFee": 2,
        "destinationAmount": 0.00066413,
        "destinationCurrencyCode": "BTC",
        "exchangeRate": 75286,
        "paymentMethodType": "APPLE_PAY",
        "customerScore": 20,
        "serviceProvider": "TRANSAK"
      }
    ],
    "message": null,
    "error": "error description"
  })",
      "US", "USD", "BTC", 50, "btc account address",
      base::BindLambdaForTesting(
          [](std::optional<std::vector<mojom::MeldCryptoQuotePtr>> quotes,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>{"error description"});
            EXPECT_FALSE(quotes);
          }));

  TestGetCryptoQuotes(
      R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [
      "[sourceAmount] must not be null",
      "[sourceCurrencyCode] must not be blank"
    ],
    "requestId": "356dd2b40fa55037bfe9d190b6438f59",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })",
      "US", "USD", "BTC", 50, "btc account address",
      base::BindLambdaForTesting(
          [&](std::optional<std::vector<mojom::MeldCryptoQuotePtr>> quotes,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>(
                                   {"[sourceAmount] must not be null",
                                    "[sourceCurrencyCode] must not be blank"}));
          }),
      net::HTTP_BAD_REQUEST);
  TestGetCryptoQuotes(
      R"({
    "quotes": null,
    "message": null,
    "error": "No Valid Quote Combinations Found For Provided Quote Request."
  })",
      "US", "USD", "BTC", 50, "btc account address",
      base::BindLambdaForTesting(
          [&](std::optional<std::vector<mojom::MeldCryptoQuotePtr>> quotes,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>(
                                   {"No Valid Quote Combinations Found For "
                                    "Provided Quote Request."}));
          }),
      net::HTTP_BAD_REQUEST);
}

TEST_F(MeldIntegrationServiceUnitTest, GetPaymentMethods) {
  auto filter = mojom::MeldFilter::New(
      "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", std::nullopt);

  const auto url = MeldIntegrationService::GetPaymentMethodsURL(filter);
  EXPECT_EQ(url.path(), "/service-providers/properties/payment-methods");
  EXPECT_EQ(url.query(),
            "accountFilter=false&includeServiceProviderDetails=false&statuses="
            "LIVE%2CRECENTLY_"
            "ADDED&countries=US%2CCA&fiatCurrencies=USD%2CEUR&cryptoCurrencies="
            "BTC%2CETH&cryptoChains=BTC%2CDOGE&serviceProviders=BANXA%"
            "2CBLOCKCHAINDOTCOM&"
            "paymentMethodTypes=MOBILE_WALLET%2CBANK_TRANSFER");
  TestGetPaymentMethods(
      R"([
  {
    "paymentMethod": "ACH",
    "name": null,
    "paymentType": "BANK_TRANSFER",
    "logos": {
      "dark": "https://images-paymentMethod.meld.io/ACH/logo_dark.png",
      "light": "https://images-paymentMethod.meld.io/ACH/logo_light.png"
    }
  }
  ])",
      "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::optional<std::vector<mojom::MeldPaymentMethodPtr>>
                 payment_methods,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_FALSE(errors.has_value());
            EXPECT_EQ(base::ranges::count_if(
                          *payment_methods,
                          [](const auto& item) {
                            return item->payment_method == "ACH" &&
                                   !item->name &&
                                   item->payment_type == "BANK_TRANSFER" &&
                                   item->logo_images &&
                                   !item->logo_images->dark_short_url &&
                                   !item->logo_images->light_short_url &&
                                   item->logo_images->dark_url ==
                                       "https://images-paymentMethod.meld.io/"
                                       "ACH/logo_dark.png" &&
                                   item->logo_images->light_url ==
                                       "https://images-paymentMethod.meld.io/"
                                       "ACH/logo_light.png";
                          }),
                      1);
          }));

  TestGetPaymentMethods(
      R"([
  {
    "paymentMethod": "ACH",
    "name": "ACH",
    "paymentType": "BANK_TRANSFER",
    "logos": {
      "dark": null,
      "light": "https://images-paymentMethod.meld.io/ACH/logo_light.png"
    }
  }
  ])",
      "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::optional<std::vector<mojom::MeldPaymentMethodPtr>>
                 payment_methods,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_FALSE(errors.has_value());
            EXPECT_EQ(base::ranges::count_if(
                          *payment_methods,
                          [](const auto& item) {
                            return item->payment_method == "ACH" &&
                                   item->name == "ACH" &&
                                   item->payment_type == "BANK_TRANSFER" &&
                                   item->logo_images &&
                                   !item->logo_images->dark_short_url &&
                                   !item->logo_images->light_short_url &&
                                   !item->logo_images->dark_url &&
                                   item->logo_images->light_url ==
                                       "https://images-paymentMethod.meld.io/"
                                       "ACH/logo_light.png";
                          }),
                      1);
          }));

  TestGetPaymentMethods(
      R"({
    "paymentMethod": "ACH",
    "name": "ACH",
    "paymentType": "BANK_TRANSFER",
    "logos": {
      "dark": null,
      "light": "https://images-paymentMethod.meld.io/ACH/logo_light.png"
    }
  })",
      "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::optional<std::vector<mojom::MeldPaymentMethodPtr>>
                 payment_methods,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});
          }));

  TestGetPaymentMethods(
      R"([{
    "name": "ACH",
    "paymentType": "BANK_TRANSFER",
    "logos": {
      "dark": null,
      "light": "https://images-paymentMethod.meld.io/ACH/logo_light.png"
    }
  }])",
      "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::optional<std::vector<mojom::MeldPaymentMethodPtr>>
                 payment_methods,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});
          }));

  TestGetPaymentMethods(
      "some wrong data", "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE",
      "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::optional<std::vector<mojom::MeldPaymentMethodPtr>>
                 payment_methods,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
          }),
      net::HTTP_REQUEST_TIMEOUT);

  TestGetPaymentMethods(
      R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [
      "[sourceAmount] must not be null",
      "[sourceCurrencyCode] must not be blank"
    ],
    "requestId": "356dd2b40fa55037bfe9d190b6438f59",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })",
      "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [&](std::optional<std::vector<mojom::MeldPaymentMethodPtr>>
                  payment_methods,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>(
                                   {"[sourceAmount] must not be null",
                                    "[sourceCurrencyCode] must not be blank"}));
          }),
      net::HTTP_BAD_REQUEST);
}

TEST_F(MeldIntegrationServiceUnitTest, GetFiatCurrencies) {
  auto filter = mojom::MeldFilter::New(
      "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", std::nullopt);
  const auto url = MeldIntegrationService::GetFiatCurrenciesURL(filter);
  EXPECT_EQ(url.path(), "/service-providers/properties/fiat-currencies");
  EXPECT_EQ(url.query(),
            "accountFilter=false&includeServiceProviderDetails=false&statuses="
            "LIVE%2CRECENTLY_"
            "ADDED&countries=US%2CCA&fiatCurrencies=USD%2CEUR&cryptoCurrencies="
            "BTC%2CETH&cryptoChains=BTC%2CDOGE&serviceProviders=BANXA%"
            "2CBLOCKCHAINDOTCOM&"
            "paymentMethodTypes=MOBILE_WALLET%2CBANK_TRANSFER");

  TestGetFiatCurrencies(
      R"([
  {
    "currencyCode": "AFN",
    "name": null,
    "symbolImageUrl": "https://images-currency.meld.io/fiat/AFN/symbol.png"
  },
  {
    "currencyCode": "DZD",
    "name": "Algerian Dinar",
    "symbolImageUrl": "https://images-currency.meld.io/fiat/DZD/symbol.png"
  }])",
      "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::optional<std::vector<mojom::MeldFiatCurrencyPtr>>
                 fiat_currencies,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_FALSE(errors.has_value());
            EXPECT_EQ(base::ranges::count_if(
                          *fiat_currencies,
                          [](const auto& item) {
                            return item->currency_code == "AFN" &&
                                   !item->name &&
                                   item->symbol_image_url ==
                                       "https://images-currency.meld.io/fiat/"
                                       "AFN/symbol.png";
                          }),
                      1);
            EXPECT_EQ(base::ranges::count_if(
                          *fiat_currencies,
                          [](const auto& item) {
                            return item->currency_code == "DZD" &&
                                   item->name == "Algerian Dinar" &&
                                   item->symbol_image_url ==
                                       "https://images-currency.meld.io/fiat/"
                                       "DZD/symbol.png";
                          }),
                      1);
          }));

  TestGetFiatCurrencies(
      R"({
    "currencyCode": "AFN",
    "name": null,
    "symbolImageUrl": "https://images-currency.meld.io/fiat/AFN/symbol.png"
  })",
      "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::optional<std::vector<mojom::MeldFiatCurrencyPtr>>
                 fiat_currencies,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});
          }));

  TestGetFiatCurrencies(
      R"([{
    "name": null,
    "symbolImageUrl": "https://images-currency.meld.io/fiat/AFN/symbol.png"
  }])",
      "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::optional<std::vector<mojom::MeldFiatCurrencyPtr>>
                 fiat_currencies,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});
          }));

  TestGetFiatCurrencies(
      "some wrong data", "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE",
      "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::optional<std::vector<mojom::MeldFiatCurrencyPtr>>
                 fiat_currencies,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
          }),
      net::HTTP_REQUEST_TIMEOUT);

  TestGetFiatCurrencies(
      R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [
      "[sourceAmount] must not be null",
      "[sourceCurrencyCode] must not be blank"
    ],
    "requestId": "356dd2b40fa55037bfe9d190b6438f59",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })",
      "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [&](std::optional<std::vector<mojom::MeldFiatCurrencyPtr>>
                  fiat_currencies,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>(
                                   {"[sourceAmount] must not be null",
                                    "[sourceCurrencyCode] must not be blank"}));
          }),
      net::HTTP_BAD_REQUEST);
}

TEST_F(MeldIntegrationServiceUnitTest, GetCryptoCurrencies) {
  auto filter = mojom::MeldFilter::New(
      "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", std::nullopt);
  const auto url = MeldIntegrationService::GetCryptoCurrenciesURL(filter);
  EXPECT_EQ(url.path(), "/service-providers/properties/crypto-currencies");
  EXPECT_EQ(url.query(),
            "accountFilter=false&includeServiceProviderDetails=false&statuses="
            "LIVE%2CRECENTLY_"
            "ADDED&countries=US%2CCA&fiatCurrencies=USD%2CEUR&cryptoCurrencies="
            "BTC%2CETH&cryptoChains=BTC%2CDOGE&serviceProviders=BANXA%"
            "2CBLOCKCHAINDOTCOM&"
            "paymentMethodTypes=MOBILE_WALLET%2CBANK_TRANSFER");

  TestGetCryptoCurrencies(
      R"([
  {
    "currencyCode": "USDT_KCC",
    "name": null,
    "chainCode": "KCC",
    "chainName": "KuCoin Community Chain",
    "chainId": "137",
    "contractAddress": "0xe41d2489571d322189246dafa5ebde1f4699f498",
    "symbolImageUrl": "https://images-currency.meld.io/crypto/USDT_KCC/symbol.png"
  },
  {
    "currencyCode": "00",
    "name": "00 Token",
    "chainCode": "ETH",
    "chainName": "Ethereum",
    "chainId": "1",
    "contractAddress": "0x111111111117dc0aa78b770fa6a738034120c302",
    "symbolImageUrl": "https://images-currency.meld.io/crypto/00/symbol.png"
  }])",
      "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "LIVE,RECENTLY_ADDED",
      base::BindLambdaForTesting(
          [](std::optional<std::vector<mojom::MeldCryptoCurrencyPtr>>
                 crypto_currencies,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_FALSE(errors.has_value());
            EXPECT_EQ(
                base::ranges::count_if(
                    *crypto_currencies,
                    [](const auto& item) {
                      return item->currency_code == "USDT_KCC" && !item->name &&
                             item->chain_code == "KCC" &&
                             item->chain_name == "KuCoin Community Chain" &&
                             item->chain_id == "0x89" &&
                             item->contract_address ==
                                 "0xe41d2489571d322189246dafa5ebde1f4699f498" &&
                             item->symbol_image_url ==
                                 "https://images-currency.meld.io/crypto/"
                                 "USDT_KCC/symbol.png";
                    }),
                1);
            EXPECT_EQ(
                base::ranges::count_if(
                    *crypto_currencies,
                    [](const auto& item) {
                      return item->currency_code == "00" &&
                             item->name == "00 Token" &&
                             item->chain_code == "ETH" &&
                             item->chain_name == "Ethereum" &&
                             item->chain_id == "0x1" &&
                             item->contract_address ==
                                 "0x111111111117dc0aa78b770fa6a738034120c302" &&
                             item->symbol_image_url ==
                                 "https://images-currency.meld.io/crypto/00/"
                                 "symbol.png";
                    }),
                1);
          }));
  TestGetCryptoCurrencies(
      R"({
    "currencyCode": "USDT_KCC",
    "name": null,
    "chainCode": "KCC",
    "chainName": "KuCoin Community Chain",
    "chainId": "0",
    "contractAddress": "0xe41d2489571d322189246dafa5ebde1f4699f498",
    "symbolImageUrl": "https://images-currency.meld.io/crypto/USDT_KCC/symbol.png"
  })",
      "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::optional<std::vector<mojom::MeldCryptoCurrencyPtr>>
                 crypto_currencies,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});
          }));

  TestGetCryptoCurrencies(
      R"([{
    "name": null,
    "chainCode": "KCC",
    "chainName": "KuCoin Community Chain",
    "chainId": "0",
    "contractAddress": "0xe41d2489571d322189246dafa5ebde1f4699f498",
    "symbolImageUrl": "https://images-currency.meld.io/crypto/USDT_KCC/symbol.png"
  }])",
      "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::optional<std::vector<mojom::MeldCryptoCurrencyPtr>>
                 crypto_currencies,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});
          }));

  TestGetCryptoCurrencies(
      "some wrong data", "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE",
      "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::optional<std::vector<mojom::MeldCryptoCurrencyPtr>>
                 crypto_currencies,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
          }),
      net::HTTP_REQUEST_TIMEOUT);

  TestGetCryptoCurrencies(
      R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [
      "[sourceAmount] must not be null",
      "[sourceCurrencyCode] must not be blank"
    ],
    "requestId": "356dd2b40fa55037bfe9d190b6438f59",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })",
      "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [&](std::optional<std::vector<mojom::MeldCryptoCurrencyPtr>>
                  crypto_currencies,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>(
                                   {"[sourceAmount] must not be null",
                                    "[sourceCurrencyCode] must not be blank"}));
          }),
      net::HTTP_BAD_REQUEST);
}

TEST_F(MeldIntegrationServiceUnitTest, GetCountries) {
  auto filter = mojom::MeldFilter::New(
      "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", std::nullopt);
  const auto url = MeldIntegrationService::GetCountriesURL(filter);
  EXPECT_EQ(url.path(), "/service-providers/properties/countries");
  EXPECT_EQ(url.query(),
            "accountFilter=false&includeServiceProviderDetails=false&statuses="
            "LIVE%2CRECENTLY_"
            "ADDED&countries=US%2CCA&fiatCurrencies=USD%2CEUR&cryptoCurrencies="
            "BTC%2CETH&cryptoChains=BTC%2CDOGE&serviceProviders=BANXA%"
            "2CBLOCKCHAINDOTCOM&"
            "paymentMethodTypes=MOBILE_WALLET%2CBANK_TRANSFER");
  TestGetCountries(
      R"([
  {
    "countryCode": "AF",
    "name": "Afghanistan",
    "flagImageUrl": "https://images-country.meld.io/AF/flag.svg",
    "regions": [
      {
        "regionCode": "CA-AB",
        "name": "Alberta"
      },
      {
        "regionCode": "CA-BC",
        "name": "British Columbia"
      }
    ]
  },
  {
    "countryCode": "AL",
    "name": "Albania",
    "flagImageUrl": "https://images-country.meld.io/AL/flag.svg",
    "regions": null
  }])",
      "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "LIVE,RECENTLY_ADDED",
      base::BindLambdaForTesting(
          [](std::optional<std::vector<mojom::MeldCountryPtr>> countries,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_FALSE(errors.has_value());
            EXPECT_EQ(
                base::ranges::count_if(
                    *countries,
                    [](const auto& item) {
                      return item->country_code == "AF" &&
                             item->name == "Afghanistan" &&
                             item->flag_image_url ==
                                 "https://images-country.meld.io/AF/flag.svg" &&
                             item->regions &&
                             (*item->regions)[0]->region_code == "CA-AB" &&
                             (*item->regions)[0]->name == "Alberta";
                    }),
                1);
            EXPECT_EQ(
                base::ranges::count_if(
                    *countries,
                    [](const auto& item) {
                      return item->country_code == "AL" &&
                             item->name == "Albania" &&
                             item->flag_image_url ==
                                 "https://images-country.meld.io/AL/flag.svg" &&
                             !item->regions;
                    }),
                1);
          }));
  TestGetCountries(
      R"([
  {
    "name": "Albania",
    "flagImageUrl": "https://images-country.meld.io/AL/flag.svg",
    "regions": null
  }])",
      "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::optional<std::vector<mojom::MeldCountryPtr>> countries,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});
          }));

  TestGetCountries(
      "some wrong data", "US,CA", "USD,EUR", "BTC,DOGE", "BTC,ETH",
      "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::optional<std::vector<mojom::MeldCountryPtr>> countries,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
          }));

  TestGetCountries(
      "some wrong data", "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE",
      "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::optional<std::vector<mojom::MeldCountryPtr>> countries,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
          }),
      net::HTTP_REQUEST_TIMEOUT);

  TestGetCountries(
      R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [
      "[sourceAmount] must not be null",
      "[sourceCurrencyCode] must not be blank"
    ],
    "requestId": "356dd2b40fa55037bfe9d190b6438f59",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })",
      "US,CA", "USD,EUR", "BTC,ETH", "BTC,DOGE", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [&](std::optional<std::vector<mojom::MeldCountryPtr>> countries,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>(
                                   {"[sourceAmount] must not be null",
                                    "[sourceCurrencyCode] must not be blank"}));
          }),
      net::HTTP_BAD_REQUEST);
}

TEST_F(MeldIntegrationServiceUnitTest, CryptoBuyWidgetCreate) {
  TestCryptoBuyWidgetCreate(
      R"({
  "id": "WXDpzmm2cNmtJWLDHgu1GT",
  "externalSessionId": "external_session_id",
  "externalCustomerId": "external_customer_id",
  "customerId": "WXEvEDzSgNedXWnJ55pwUJ",
  "widgetUrl": "https://meldcrypto.com?token=token-val",
  "token": "token-val"
})",
      "US", "BTC", std::vector<std::string>{"cryptoCurrency"}, "BANK_TRANSFER",
      "https://sb.fluidmoney.xyz", "AKOYA", "100", "USD", "testWalletAddress",
      "walletTag", "customer@email", "customer_id", "external_customer_id",
      "external_session_id",
      base::BindLambdaForTesting(
          [](mojom::MeldCryptoWidgetPtr crypto_widget,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_FALSE(errors.has_value());
            EXPECT_EQ("WXDpzmm2cNmtJWLDHgu1GT", crypto_widget->id);
            EXPECT_EQ("external_customer_id",
                      crypto_widget->external_customer_id);
            EXPECT_EQ("external_session_id",
                      crypto_widget->external_session_id);
            EXPECT_EQ("WXEvEDzSgNedXWnJ55pwUJ", crypto_widget->customer_id);
            EXPECT_EQ("token-val", crypto_widget->token);
            EXPECT_EQ("https://meldcrypto.com?token=token-val",
                      crypto_widget->widget_url);
          }));

  TestCryptoBuyWidgetCreate(
      R"({
  "externalSessionId": "external_session_id",
  "externalCustomerId": "external_customer_id",
  "customerId": "WXEvEDzSgNedXWnJ55pwUJ",
  "widgetUrl": "https://meldcrypto.com?token=token-val",
  "token": "token-val"
})",
      "US", "BTC", std::vector<std::string>{"cryptoCurrency"}, "BANK_TRANSFER",
      "https://sb.fluidmoney.xyz", "AKOYA", "100", "USD", "testWalletAddress",
      "walletTag", std::nullopt, std::nullopt, std::nullopt, std::nullopt,
      base::BindLambdaForTesting(
          [](mojom::MeldCryptoWidgetPtr crypto_widget,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});
          }));

  TestCryptoBuyWidgetCreate(
      "some wrong data", "US", "BTC",
      std::vector<std::string>{"cryptoCurrency"}, "BANK_TRANSFER",
      "https://sb.fluidmoney.xyz", "AKOYA", "100", "USD", "testWalletAddress",
      "walletTag", std::nullopt, std::nullopt, std::nullopt, std::nullopt,
      base::BindLambdaForTesting(
          [](mojom::MeldCryptoWidgetPtr crypto_widget,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
          }));
  TestCryptoBuyWidgetCreate(
      "some wrong data", "US", "BTC",
      std::vector<std::string>{"cryptoCurrency"}, "BANK_TRANSFER",
      "https://sb.fluidmoney.xyz", "AKOYA", "100", "USD", "testWalletAddress",
      "walletTag", std::nullopt, std::nullopt, std::nullopt, std::nullopt,
      base::BindLambdaForTesting(
          [](mojom::MeldCryptoWidgetPtr crypto_widget,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
          }),
      net::HTTP_REQUEST_TIMEOUT);

  TestCryptoBuyWidgetCreate(
      "", "", "", std::nullopt, std::nullopt, std::nullopt, "", "", "", "",
      std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
      base::BindLambdaForTesting(
          [](mojom::MeldCryptoWidgetPtr crypto_widget,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{l10n_util::GetStringUTF8(
                          IDS_WALLET_REQUEST_PROCESSING_ERROR)});
          }),
      net::HTTP_OK, true);

  TestCryptoBuyWidgetCreate(
      R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [
      "[sourceAmount] must not be null",
      "[sourceCurrencyCode] must not be blank"
    ],
    "requestId": "356dd2b40fa55037bfe9d190b6438f59",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })",
      "US", "BTC", std::vector<std::string>{"cryptoCurrency"}, "BANK_TRANSFER",
      "https://sb.fluidmoney.xyz", "AKOYA", "100", "USD", "testWalletAddress",
      "walletTag", std::nullopt, std::nullopt, std::nullopt, std::nullopt,
      base::BindLambdaForTesting(
          [](mojom::MeldCryptoWidgetPtr crypto_widget,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>(
                                   {"[sourceAmount] must not be null",
                                    "[sourceCurrencyCode] must not be blank"}));
          }),
      net::HTTP_BAD_REQUEST);
}

TEST_F(MeldIntegrationServiceUnitTest, CryptoSellWidgetCreate) {
  TestCryptoSellWidgetCreate(
      R"({
  "id": "WXDpzmm2cNmtJWLDHgu1GT",
  "externalSessionId": "external_session_id",
  "externalCustomerId": "external_customer_id",
  "customerId": "WXEvEDzSgNedXWnJ55pwUJ",
  "widgetUrl": "https://meldcrypto.com?token=token-val",
  "token": "token-val"
})",
      "US", "BTC", std::vector<std::string>{"cryptoCurrency"}, "BANK_TRANSFER",
      "https://sb.fluidmoney.xyz", "AKOYA", "100", "USD", std::nullopt,
      "walletTag", "customer@email", "customer_id", "external_customer_id",
      "external_session_id",
      base::BindLambdaForTesting(
          [](mojom::MeldCryptoWidgetPtr crypto_widget,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_FALSE(errors.has_value());
            EXPECT_EQ("WXDpzmm2cNmtJWLDHgu1GT", crypto_widget->id);
            EXPECT_EQ("external_customer_id",
                      crypto_widget->external_customer_id);
            EXPECT_EQ("external_session_id",
                      crypto_widget->external_session_id);
            EXPECT_EQ("WXEvEDzSgNedXWnJ55pwUJ", crypto_widget->customer_id);
            EXPECT_EQ("token-val", crypto_widget->token);
            EXPECT_EQ("https://meldcrypto.com?token=token-val",
                      crypto_widget->widget_url);
          }));

  TestCryptoSellWidgetCreate(
      R"({
  "externalSessionId": "external_session_id",
  "externalCustomerId": "external_customer_id",
  "customerId": "WXEvEDzSgNedXWnJ55pwUJ",
  "widgetUrl": "https://meldcrypto.com?token=token-val",
  "token": "token-val"
})",
      "US", "BTC", std::vector<std::string>{"cryptoCurrency"}, "BANK_TRANSFER",
      "https://sb.fluidmoney.xyz", "AKOYA", "100", "USD", "testWalletAddress",
      "walletTag", std::nullopt, std::nullopt, std::nullopt, std::nullopt,
      base::BindLambdaForTesting(
          [](mojom::MeldCryptoWidgetPtr crypto_widget,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});
          }));

  TestCryptoSellWidgetCreate(
      "some wrong data", "US", "BTC",
      std::vector<std::string>{"cryptoCurrency"}, "BANK_TRANSFER",
      "https://sb.fluidmoney.xyz", "AKOYA", "100", "USD", "testWalletAddress",
      "walletTag", std::nullopt, std::nullopt, std::nullopt, std::nullopt,
      base::BindLambdaForTesting(
          [](mojom::MeldCryptoWidgetPtr crypto_widget,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
          }));
  TestCryptoSellWidgetCreate(
      "some wrong data", "US", "BTC",
      std::vector<std::string>{"cryptoCurrency"}, "BANK_TRANSFER",
      "https://sb.fluidmoney.xyz", "AKOYA", "100", "USD", "testWalletAddress",
      "walletTag", std::nullopt, std::nullopt, std::nullopt, std::nullopt,
      base::BindLambdaForTesting(
          [](mojom::MeldCryptoWidgetPtr crypto_widget,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
          }),
      net::HTTP_REQUEST_TIMEOUT);

  TestCryptoSellWidgetCreate(
      "", "", "", std::nullopt, std::nullopt, std::nullopt, "", "", "",
      std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
      std::nullopt,
      base::BindLambdaForTesting(
          [](mojom::MeldCryptoWidgetPtr crypto_widget,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{l10n_util::GetStringUTF8(
                          IDS_WALLET_REQUEST_PROCESSING_ERROR)});
          }),
      net::HTTP_OK, true);

  TestCryptoSellWidgetCreate(
      R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [
      "[sourceAmount] must not be null",
      "[sourceCurrencyCode] must not be blank"
    ],
    "requestId": "356dd2b40fa55037bfe9d190b6438f59",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })",
      "US", "BTC", std::vector<std::string>{"cryptoCurrency"}, "BANK_TRANSFER",
      "https://sb.fluidmoney.xyz", "AKOYA", "100", "USD", "testWalletAddress",
      "walletTag", std::nullopt, std::nullopt, std::nullopt,
      "external_session_id",
      base::BindLambdaForTesting(
          [](mojom::MeldCryptoWidgetPtr crypto_widget,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>(
                                   {"[sourceAmount] must not be null",
                                    "[sourceCurrencyCode] must not be blank"}));
          }),
      net::HTTP_BAD_REQUEST);
}

TEST_F(MeldIntegrationServiceUnitTest, CryptoTransferWidgetCreate) {
  TestCryptoTransferWidgetCreate(
      R"({
  "id": "WXDpzmm2cNmtJWLDHgu1GT",
  "externalSessionId": "external_session_id",
  "externalCustomerId": "external_customer_id",
  "customerId": "WXEvEDzSgNedXWnJ55pwUJ",
  "widgetUrl": "https://meldcrypto.com?token=token-val",
  "token": "token-val"
})",
      "US", "institution", std::vector<std::string>{"cryptoCurrency"},
      "https://sb.fluidmoney.xyz", "AKOYA", "1",
      std::vector<std::string>{"BTC", "ETH"}, std::nullopt, "walletTag",
      "customer@email", "customer_id", "external_customer_id",
      "external_session_id",
      base::BindLambdaForTesting(
          [](mojom::MeldCryptoWidgetPtr crypto_widget,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_FALSE(errors.has_value());
            EXPECT_EQ("WXDpzmm2cNmtJWLDHgu1GT", crypto_widget->id);
            EXPECT_EQ("external_customer_id",
                      crypto_widget->external_customer_id);
            EXPECT_EQ("external_session_id",
                      crypto_widget->external_session_id);
            EXPECT_EQ("WXEvEDzSgNedXWnJ55pwUJ", crypto_widget->customer_id);
            EXPECT_EQ("token-val", crypto_widget->token);
            EXPECT_EQ("https://meldcrypto.com?token=token-val",
                      crypto_widget->widget_url);
          }));
  TestCryptoTransferWidgetCreate(
      R"({
  "externalSessionId": "external_session_id",
  "externalCustomerId": "external_customer_id",
  "customerId": "WXEvEDzSgNedXWnJ55pwUJ",
  "widgetUrl": "https://meldcrypto.com?token=token-val",
  "token": "token-val"
})",
      "US", "institution", std::vector<std::string>{"cryptoCurrency"},
      "https://sb.fluidmoney.xyz", "AKOYA", "1",
      std::vector<std::string>{"BTC", "ETH"}, std::nullopt, "walletTag",
      std::nullopt, std::nullopt, std::nullopt, std::nullopt,
      base::BindLambdaForTesting(
          [](mojom::MeldCryptoWidgetPtr crypto_widget,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});
          }));

  TestCryptoTransferWidgetCreate(
      "some wrong data", "US", "institution",
      std::vector<std::string>{"cryptoCurrency"}, "https://sb.fluidmoney.xyz",
      "AKOYA", "1", std::vector<std::string>{"BTC", "ETH"}, std::nullopt,
      "walletTag", std::nullopt, std::nullopt, std::nullopt, std::nullopt,
      base::BindLambdaForTesting(
          [](mojom::MeldCryptoWidgetPtr crypto_widget,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
          }));
  TestCryptoTransferWidgetCreate(
      "some wrong data", "US", "institution",
      std::vector<std::string>{"cryptoCurrency"}, "https://sb.fluidmoney.xyz",
      "AKOYA", "1", std::vector<std::string>{"BTC", "ETH"}, std::nullopt,
      "walletTag", std::nullopt, std::nullopt, std::nullopt, std::nullopt,
      base::BindLambdaForTesting(
          [](mojom::MeldCryptoWidgetPtr crypto_widget,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
          }),
      net::HTTP_REQUEST_TIMEOUT);

  TestCryptoTransferWidgetCreate(
      "", std::nullopt, std::nullopt, std::nullopt, std::nullopt, "", "",
      std::vector<std::string>{}, std::nullopt, std::nullopt, std::nullopt,
      std::nullopt, std::nullopt, std::nullopt,
      base::BindLambdaForTesting(
          [](mojom::MeldCryptoWidgetPtr crypto_widget,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{l10n_util::GetStringUTF8(
                          IDS_WALLET_REQUEST_PROCESSING_ERROR)});
          }),
      net::HTTP_OK, true);

  TestCryptoTransferWidgetCreate(
      R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [
      "[sourceAmount] must not be null",
      "[sourceCurrencyCode] must not be blank"
    ],
    "requestId": "356dd2b40fa55037bfe9d190b6438f59",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })",
      "US", "institution", std::vector<std::string>{"cryptoCurrency"},
      "https://sb.fluidmoney.xyz", "AKOYA", "1",
      std::vector<std::string>{"BTC", "ETH"}, std::nullopt, "walletTag",
      "customer@email", "customer_id", "external_customer_id",
      "external_session_id",
      base::BindLambdaForTesting(
          [](mojom::MeldCryptoWidgetPtr crypto_widget,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>(
                                   {"[sourceAmount] must not be null",
                                    "[sourceCurrencyCode] must not be blank"}));
          }),
      net::HTTP_BAD_REQUEST);
}

}  // namespace brave_wallet
