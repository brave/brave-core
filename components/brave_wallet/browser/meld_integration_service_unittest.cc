/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/meld_integration_service.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/test/test_url_loader_factory.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace brave_wallet {

class MeldIntegrationServiceUnitTest : public testing::Test {
 public:
  MeldIntegrationServiceUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {
    asset_ratio_service_ =
        std::make_unique<MeldIntegrationService>(shared_url_loader_factory_);
  }

  ~MeldIntegrationServiceUnitTest() override = default;

  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory() {
    return shared_url_loader_factory_;
  }

  void SetInterceptor(const std::string& content,
                      const std::string expected_header = "") {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, content, expected_header](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          std::string header;
          request.headers.GetHeader("Authorization", &header);
          url_loader_factory_.AddResponse(request.url.spec(), content);
        }));
  }

  void SetErrorInterceptor(const std::string& content) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, content](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), content,
                                          net::HTTP_REQUEST_TIMEOUT);
        }));
  }

  void TestGetServiceProvider(
      const std::string& content,
      const std::string& countries,
      const std::string& from_assets,
      const std::string& to_assets,
      const std::string& service_providers,
      const std::string& payment_method_types,
      const std::string& statuses,
      MeldIntegrationService::GetServiceProvidersCallback callback,
      const bool error_interceptor = false) {
    if (!error_interceptor) {
      SetInterceptor(content);
    } else {
      SetErrorInterceptor("error");
    }
    base::RunLoop run_loop;
    asset_ratio_service_->GetServiceProviders(
        countries, from_assets, to_assets, service_providers,
        payment_method_types, statuses,
        base::BindLambdaForTesting(
            [&](std::optional<std::vector<mojom::ServiceProviderPtr>> sps,
                const std::optional<std::vector<std::string>>& errors) {
              std::move(callback).Run(std::move(sps), errors);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestGetCryptoQuotes(const std::string& content,
                           const std::string& country,
                           const std::string& from_asset,
                           const std::string& to_asset,
                           const double source_amount,
                           const std::string& account,
                           MeldIntegrationService::GetCryptoQuotesCallback callback,
                           const bool error_interceptor = false) {
    if (!error_interceptor) {
      SetInterceptor(content);
    } else {
      SetErrorInterceptor("error");
    }
    base::RunLoop run_loop;
    asset_ratio_service_->GetCryptoQuotes(
        country, from_asset, to_asset, source_amount, account,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::CryptoQuotePtr> quotes,
                const std::optional<std::vector<std::string>>& errors) {
              std::move(callback).Run(std::move(quotes), errors);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestGetPaymentMethods(
      const std::string& content,
      const std::string& countries,
      const std::string& fiat_currencies,
      const std::string& crypto_currencies,
      const std::string& service_providers,
      const std::string& payment_method_types,
      const std::string& statuses,
      MeldIntegrationService::GetPaymentMethodsCallback callback,
      const bool error_interceptor = false) {
    if (!error_interceptor) {
      SetInterceptor(content);
    } else {
      SetErrorInterceptor("error");
    }
    base::RunLoop run_loop;
    asset_ratio_service_->GetPaymentMethods(
        countries, fiat_currencies, crypto_currencies, service_providers,
        payment_method_types, statuses,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::PaymentMethodPtr> payment_methods,
                const std::optional<std::vector<std::string>>& errors) {
              std::move(callback).Run(std::move(payment_methods), errors);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestGetFiatCurrencies(
      const std::string& content,
      const std::string& countries,
      const std::string& fiat_currencies,
      const std::string& crypto_currencies,
      const std::string& service_providers,
      const std::string& payment_method_types,
      const std::string& statuses,
      MeldIntegrationService::GetFiatCurrenciesCallback callback,
      const bool error_interceptor = false) {
    if (!error_interceptor) {
      SetInterceptor(content);
    } else {
      SetErrorInterceptor("error");
    }
    base::RunLoop run_loop;
    asset_ratio_service_->GetFiatCurrencies(
        countries, fiat_currencies, crypto_currencies, service_providers,
        payment_method_types, statuses,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::FiatCurrencyPtr> fiat_currencies,
                const std::optional<std::vector<std::string>>& errors) {
              std::move(callback).Run(std::move(fiat_currencies), errors);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestGetCryptoCurrencies(
      const std::string& content,
      const std::string& countries,
      const std::string& fiat_currencies,
      const std::string& crypto_currencies,
      const std::string& service_providers,
      const std::string& payment_method_types,
      const std::string& statuses,
      MeldIntegrationService::GetCryptoCurrenciesCallback callback,
      const bool error_interceptor = false) {
    if (!error_interceptor) {
      SetInterceptor(content);
    } else {
      SetErrorInterceptor("error");
    }
    base::RunLoop run_loop;
    asset_ratio_service_->GetCryptoCurrencies(
        countries, fiat_currencies, crypto_currencies, service_providers,
        payment_method_types, statuses,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::CryptoCurrencyPtr> crypto_currencies,
                const std::optional<std::vector<std::string>>& errors) {
              std::move(callback).Run(std::move(crypto_currencies), errors);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestGetCountries(const std::string& content,
                        const std::string& countries,
                        const std::string& fiat_currencies,
                        const std::string& crypto_currencies,
                        const std::string& service_providers,
                        const std::string& payment_method_types,
                        const std::string& statuses,
                        MeldIntegrationService::GetCountriesCallback callback,
                        const bool error_interceptor = false) {
    if (!error_interceptor) {
      SetInterceptor(content);
    } else {
      SetErrorInterceptor("error");
    }
    base::RunLoop run_loop;
    asset_ratio_service_->GetCountries(
        countries, fiat_currencies, crypto_currencies, service_providers,
        payment_method_types, statuses,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::CountryPtr> countries,
                const std::optional<std::vector<std::string>>& errors) {
              std::move(callback).Run(std::move(countries), errors);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

 protected:
  std::unique_ptr<MeldIntegrationService> asset_ratio_service_;
  base::test::TaskEnvironment task_environment_;

 private:
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  //  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(MeldIntegrationServiceUnitTest, GetServiceProviders) {
  const auto url = MeldIntegrationService::GetServiceProviderURL(
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", std::nullopt);
  EXPECT_EQ(url.path(), "/service-providers");
  EXPECT_EQ(url.query(),
            "accountFilter=false&statuses=LIVE%2CRECENTLY_ADDED&countries=US%"
            "2CCA&fiatCurrencies=USD%2CEUR&cryptoCurrencies=BTC%2CETH&"
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
      "lightShort": "https://images-serviceprovider.meld.io/BANXA/short_logo_light.png"
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
    "logos": {
      "dark": "https://images-serviceprovider.meld.io/BLOCKCHAINDOTCOM/logo_dark.png",
      "light": "https://images-serviceprovider.meld.io/BLOCKCHAINDOTCOM/logo_light.png",
      "darkShort": "https://images-serviceprovider.meld.io/BLOCKCHAINDOTCOM/short_logo_dark.png",
      "lightShort": "https://images-serviceprovider.meld.io/BLOCKCHAINDOTCOM/short_logo_light.png"
    }
  }])",
      "US", "USD", "ETH", "", "", "",
      base::BindLambdaForTesting(
          [&](std::optional<std::vector<mojom::ServiceProviderPtr>> sps,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_FALSE(errors.has_value());
            EXPECT_EQ(base::ranges::count_if(
                          *sps,
                          [](const auto& item) {
                            return item->name == "Banxa" &&
                                   item->service_provider == "BANXA" &&
                                   item->status == "LIVE" &&
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
                                   item->logo_images->light_short_url ==
                                       "https://images-serviceprovider.meld.io/"
                                       "BANXA/short_logo_light.png";
                          }),
                      1);
            EXPECT_EQ(base::ranges::count_if(
                          *sps,
                          [](const auto& item) {
                            return item->name == "Blockchain.com" &&
                                   item->service_provider ==
                                       "BLOCKCHAINDOTCOM" &&
                                   item->status == "LIVE" &&
                                   item->web_site_url ==
                                       "https://www.blockchain.com" &&
                                   item->logo_images &&
                                   item->logo_images->dark_url ==
                                       "https://images-serviceprovider.meld.io/"
                                       "BLOCKCHAINDOTCOM/logo_dark.png" &&
                                   item->logo_images->dark_short_url ==
                                       "https://images-serviceprovider.meld.io/"
                                       "BLOCKCHAINDOTCOM/short_logo_dark.png" &&
                                   item->logo_images->light_url ==
                                       "https://images-serviceprovider.meld.io/"
                                       "BLOCKCHAINDOTCOM/logo_light.png" &&
                                   item->logo_images->light_short_url ==
                                       "https://images-serviceprovider.meld.io/"
                                       "BLOCKCHAINDOTCOM/short_logo_light.png";
                          }),
                      1);
          }));
  TestGetServiceProvider(
      "some wrone data", "US", "USD", "ETH", "", "", "",
      base::BindLambdaForTesting(
          [&](std::optional<std::vector<mojom::ServiceProviderPtr>> sps,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>{"PARSING_ERROR"});
          }));
  TestGetServiceProvider(
      "some wrone data", "US", "USD", "ETH", "", "", "",
      base::BindLambdaForTesting(
          [&](std::optional<std::vector<mojom::ServiceProviderPtr>> sps,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
          }),
      true);
  TestGetServiceProvider(
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
      "US", "USD", "ETH", "", "", "",
      base::BindLambdaForTesting(
          [&](std::optional<std::vector<mojom::ServiceProviderPtr>> sps,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>(
                                   {"[sourceAmount] must not be null",
                                    "[sourceCurrencyCode] must not be blank"}));
          }),
      false);
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
          [](std::vector<mojom::CryptoQuotePtr> quotes,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_FALSE(errors.has_value());
            EXPECT_EQ(base::ranges::count_if(
                          quotes,
                          [](const auto& item) {
                          return item->transaction_type == "CRYPTO_PURCHASE" &&
                                item->source_amount == 50 &&
                                item->source_amount_without_fee == 43.97 &&
                                item->fiat_amount_without_fees == 43.97 &&
                                item->destination_amount_without_fees == std::nullopt &&
                                item->source_currency_code == "USD" &&
                                item->country_code == "US" &&
                                item->total_fee == 6.03 &&
                                item->network_fee == 3.53 &&
                                item->transaction_fee == 2 &&
                                item->destination_amount == 0.00066413 &&
                                item->destination_currency_code == "BTC" &&
                                item->exchange_rate == 75286 &&
                                item->payment_method == "APPLE_PAY" &&
                                item->customer_score == 20 &&
                                item->service_provider == "TRANSAK";
                          }),
                      1);
          }));

  TestGetCryptoQuotes(
      "some wrong data", "US", "USD", "BTC", 50, "btc account address",
      base::BindLambdaForTesting(
          [](std::vector<mojom::CryptoQuotePtr> quotes,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>{"PARSING_ERROR"});
          }));

  TestGetCryptoQuotes(
      "some wrong data", "US", "USD", "BTC", 50, "btc account address",
      base::BindLambdaForTesting(
          [](std::vector<mojom::CryptoQuotePtr> quotes,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
          }),
      true);

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
  "error": "error description"
})",
      "US", "USD", "BTC", 50, "btc account address",
      base::BindLambdaForTesting(
          [](std::vector<mojom::CryptoQuotePtr> quotes,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>{"error description"});
            EXPECT_EQ(base::ranges::count_if(
                          quotes,
                          [](const auto& item) {
                          return item->transaction_type == "CRYPTO_PURCHASE" &&
                                item->source_amount == 50 &&
                                item->source_amount_without_fee == 43.97 &&
                                item->fiat_amount_without_fees == 43.97 &&
                                item->destination_amount_without_fees == std::nullopt &&
                                item->source_currency_code == "USD" &&
                                item->country_code == "US" &&
                                item->total_fee == 6.03 &&
                                item->network_fee == 3.53 &&
                                item->transaction_fee == 2 &&
                                item->destination_amount == 0.00066413 &&
                                item->destination_currency_code == "BTC" &&
                                item->exchange_rate == 75286 &&
                                item->payment_method == "APPLE_PAY" &&
                                item->customer_score == 20 &&
                                item->service_provider == "TRANSAK";
                          }),
                      1);
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
          [&](std::vector<mojom::CryptoQuotePtr> quotes,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>(
                                   {"[sourceAmount] must not be null",
                                    "[sourceCurrencyCode] must not be blank"}));
          }),
      false);
}

TEST_F(MeldIntegrationServiceUnitTest, GetPaymentMethods) {
  const auto url = MeldIntegrationService::GetPaymentMethodsURL(
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", std::nullopt);
  EXPECT_EQ(url.path(), "/service-providers/properties/payment-methods");
  EXPECT_EQ(url.query(),
            "includeServiceProviderDetails=false&statuses=LIVE%2CRECENTLY_"
            "ADDED&countries=US%2CCA&fiatCurrencies=USD%2CEUR&cryptoCurrencies="
            "BTC%2CETH&serviceProviders=BANXA%2CBLOCKCHAINDOTCOM&"
            "paymentMethodTypes=MOBILE_WALLET%2CBANK_TRANSFER");
  TestGetPaymentMethods(
      R"([
  {
    "paymentMethod": "ACH",
    "name": "ACH",
    "paymentType": "BANK_TRANSFER",
    "logos": {
      "dark": "https://images-paymentMethod.meld.io/ACH/logo_dark.png",
      "light": "https://images-paymentMethod.meld.io/ACH/logo_light.png"
    }
  }
  ])",
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::vector<mojom::PaymentMethodPtr> payment_methods,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_FALSE(errors.has_value());
            EXPECT_EQ(base::ranges::count_if(
                          payment_methods,
                          [](const auto& item) {
                            return item->payment_method == "ACH" &&
                                    item->name == "ACH" &&
                                    item->payment_type == "BANK_TRANSFER" &&
                                    item->logo_images &&
                                    !item->logo_images->dark_short_url &&
                                    !item->logo_images->light_short_url &&
                                   item->logo_images->dark_url ==
                                       "https://images-paymentMethod.meld.io/"
                                       "ACH/logo_dark.png" &&
                                   item->logo_images->light_url ==
                                       "https://images-paymentMethod.meld.io/"
                                       "ACH/logo_light.png"
                            ;
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
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::vector<mojom::PaymentMethodPtr> payment_methods,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_FALSE(errors.has_value());
            EXPECT_EQ(base::ranges::count_if(
                          payment_methods,
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
                                       "ACH/logo_light.png"
                            ;
                          }),
                      1);
          }));

  TestGetPaymentMethods(
      "some wrong data", "US,CA", "USD,EUR", "BTC,ETH",
      "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::vector<mojom::PaymentMethodPtr> payment_methods,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>{"PARSING_ERROR"});
          }));

  TestGetPaymentMethods(
      "some wrong data", "US,CA", "USD,EUR", "BTC,ETH",
      "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::vector<mojom::PaymentMethodPtr> payment_methods,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
          }),
      true);

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
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [&](std::vector<mojom::PaymentMethodPtr> payment_methods,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>(
                                   {"[sourceAmount] must not be null",
                                    "[sourceCurrencyCode] must not be blank"}));
          }),
      false);
}

TEST_F(MeldIntegrationServiceUnitTest, GetFiatCurrencies) {
  const auto url = MeldIntegrationService::GetFiatCurrenciesURL(
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", std::nullopt);
  EXPECT_EQ(url.path(), "/service-providers/properties/fiat-currencies");
  EXPECT_EQ(url.query(),
            "includeServiceProviderDetails=false&statuses=LIVE%2CRECENTLY_"
            "ADDED&countries=US%2CCA&fiatCurrencies=USD%2CEUR&cryptoCurrencies="
            "BTC%2CETH&serviceProviders=BANXA%2CBLOCKCHAINDOTCOM&"
            "paymentMethodTypes=MOBILE_WALLET%2CBANK_TRANSFER");

  TestGetFiatCurrencies(
      R"([
  {
    "currencyCode": "AFN",
    "name": "Afghani",
    "symbolImageUrl": "https://images-currency.meld.io/fiat/AFN/symbol.png"
  },
  {
    "currencyCode": "DZD",
    "name": "Algerian Dinar",
    "symbolImageUrl": "https://images-currency.meld.io/fiat/DZD/symbol.png"
  }])",
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::vector<mojom::FiatCurrencyPtr> fiat_currencies,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_FALSE(errors.has_value());
            EXPECT_EQ(base::ranges::count_if(
                          fiat_currencies,
                          [](const auto& item) {
                            return item->currency_code == "AFN" &&
                                   item->name == "Afghani" &&
                                   item->symbol_image_url ==
                                       "https://images-currency.meld.io/fiat/"
                                       "AFN/symbol.png";
                          }),
                      1);
            EXPECT_EQ(base::ranges::count_if(
                          fiat_currencies,
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
      "some wrong data", "US,CA", "USD,EUR", "BTC,ETH",
      "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::vector<mojom::FiatCurrencyPtr> fiat_currencies,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>{"PARSING_ERROR"});
          }));

  TestGetFiatCurrencies(
      "some wrong data", "US,CA", "USD,EUR", "BTC,ETH",
      "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::vector<mojom::FiatCurrencyPtr> fiat_currencies,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
          }),
      true);

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
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [&](std::vector<mojom::FiatCurrencyPtr> fiat_currencies,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>(
                                   {"[sourceAmount] must not be null",
                                    "[sourceCurrencyCode] must not be blank"}));
          }),
      false);
}

TEST_F(MeldIntegrationServiceUnitTest, GetCryptoCurrencies) {
  const auto url = MeldIntegrationService::GetCryptoCurrenciesURL(
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", std::nullopt);
  EXPECT_EQ(url.path(), "/service-providers/properties/crypto-currencies");
  EXPECT_EQ(url.query(),
            "includeServiceProviderDetails=false&statuses=LIVE%2CRECENTLY_"
            "ADDED&countries=US%2CCA&fiatCurrencies=USD%2CEUR&cryptoCurrencies="
            "BTC%2CETH&serviceProviders=BANXA%2CBLOCKCHAINDOTCOM&"
            "paymentMethodTypes=MOBILE_WALLET%2CBANK_TRANSFER");

  TestGetCryptoCurrencies(
      R"([
  {
    "currencyCode": "USDT_KCC",
    "name": "#REF!",
    "chainCode": "KCC",
    "chainName": "KuCoin Community Chain",
    "chainId": "0",
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
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "LIVE,RECENTLY_ADDED",
      base::BindLambdaForTesting(
          [](std::vector<mojom::CryptoCurrencyPtr> crypto_currencies,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_FALSE(errors.has_value());
            EXPECT_EQ(
                base::ranges::count_if(
                    crypto_currencies,
                    [](const auto& item) {
                      return item->currency_code == "USDT_KCC" &&
                             item->name == "#REF!" &&
                             item->chain_code == "KCC" &&
                             item->chain_name == "KuCoin Community Chain" &&
                             item->chain_id == "0" &&
                             item->contract_address ==
                                 "0xe41d2489571d322189246dafa5ebde1f4699f498" &&
                             item->symbol_image_url ==
                                 "https://images-currency.meld.io/crypto/"
                                 "USDT_KCC/symbol.png";
                    }),
                1);
            EXPECT_EQ(
                base::ranges::count_if(
                    crypto_currencies,
                    [](const auto& item) {
                      return item->currency_code == "00" &&
                             item->name == "00 Token" &&
                             item->chain_code == "ETH" &&
                             item->chain_name == "Ethereum" &&
                             item->chain_id == "1" &&
                             item->contract_address ==
                                 "0x111111111117dc0aa78b770fa6a738034120c302" &&
                             item->symbol_image_url ==
                                 "https://images-currency.meld.io/crypto/00/"
                                 "symbol.png";
                    }),
                1);
          }));
  TestGetCryptoCurrencies(
      "some wrong data", "US,CA", "USD,EUR", "BTC,ETH",
      "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::vector<mojom::CryptoCurrencyPtr> crypto_currencies,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>{"PARSING_ERROR"});
          }));

  TestGetCryptoCurrencies(
      "some wrong data", "US,CA", "USD,EUR", "BTC,ETH",
      "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::vector<mojom::CryptoCurrencyPtr> crypto_currencies,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
          }),
      true);

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
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [&](std::vector<mojom::CryptoCurrencyPtr> crypto_currencies,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>(
                                   {"[sourceAmount] must not be null",
                                    "[sourceCurrencyCode] must not be blank"}));
          }),
      false);
}

TEST_F(MeldIntegrationServiceUnitTest, GetCountries) {
  const auto url = MeldIntegrationService::GetCountriesURL(
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", std::nullopt);
  EXPECT_EQ(url.path(), "/service-providers/properties/countries");
  EXPECT_EQ(url.query(),
            "includeServiceProviderDetails=false&statuses=LIVE%2CRECENTLY_"
            "ADDED&countries=US%2CCA&fiatCurrencies=USD%2CEUR&cryptoCurrencies="
            "BTC%2CETH&serviceProviders=BANXA%2CBLOCKCHAINDOTCOM&"
            "paymentMethodTypes=MOBILE_WALLET%2CBANK_TRANSFER");
  TestGetCountries(
      R"([
  {
    "countryCode": "AF",
    "name": "Afghanistan",
    "flagImageUrl": "https://images-country.meld.io/AF/flag.svg",
    "regions": null
  },
  {
    "countryCode": "AL",
    "name": "Albania",
    "flagImageUrl": "https://images-country.meld.io/AL/flag.svg",
    "regions": null
  }])",
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "LIVE,RECENTLY_ADDED",
      base::BindLambdaForTesting(
          [](std::vector<mojom::CountryPtr> countries,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_FALSE(errors.has_value());
            EXPECT_EQ(
                base::ranges::count_if(
                    countries,
                    [](const auto& item) {
                      return item->country_code == "AF" &&
                             item->name == "Afghanistan" &&
                             item->flag_image_url ==
                                 "https://images-country.meld.io/AF/flag.svg";
                    }),
                1);
            EXPECT_EQ(
                base::ranges::count_if(
                    countries,
                    [](const auto& item) {
                      return item->country_code == "AL" &&
                             item->name == "Albania" &&
                             item->flag_image_url ==
                                 "https://images-country.meld.io/AL/flag.svg";
                    }),
                1);
          }));
  TestGetCountries(
      "some wrong data", "US,CA", "USD,EUR", "BTC,ETH",
      "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::vector<mojom::CountryPtr> countries,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>{"PARSING_ERROR"});
          }));

  TestGetCountries(
      "some wrong data", "US,CA", "USD,EUR", "BTC,ETH",
      "BANXA,BLOCKCHAINDOTCOM", "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [](std::vector<mojom::CountryPtr> countries,
             const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors,
                      std::vector<std::string>{"INTERNAL_SERVICE_ERROR"});
          }),
      true);

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
      "US,CA", "USD,EUR", "BTC,ETH", "BANXA,BLOCKCHAINDOTCOM",
      "MOBILE_WALLET,BANK_TRANSFER", "",
      base::BindLambdaForTesting(
          [&](std::vector<mojom::CountryPtr> countries,
              const std::optional<std::vector<std::string>>& errors) {
            EXPECT_TRUE(errors.has_value());
            EXPECT_EQ(*errors, std::vector<std::string>(
                                   {"[sourceAmount] must not be null",
                                    "[sourceCurrencyCode] must not be blank"}));
          }),
      false);
}

}  // namespace brave_wallet
