/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/meld_integration_service.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/meld_integration.mojom.h"
#include "components/grit/brave_components_strings.h"
#include "net/base/url_search_params.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

using base::test::IsJson;
using base::test::TestFuture;
using testing::ElementsAre;
using testing::Pair;

}  // namespace

namespace brave_wallet {

class MeldIntegrationServiceUnitTest : public testing::Test {
 public:
  MeldIntegrationServiceUnitTest() {
    meld_integration_service_ = std::make_unique<MeldIntegrationService>(
        url_loader_factory_.GetSafeWeakWrapper());
  }

  ~MeldIntegrationServiceUnitTest() override = default;

  using Errors = std::optional<std::vector<std::string>>;
  using ServiceProviders =
      std::optional<std::vector<mojom::MeldServiceProviderPtr>>;
  using CryptoQuotes = std::optional<std::vector<mojom::MeldCryptoQuotePtr>>;
  using PaymentMethods =
      std::optional<std::vector<mojom::MeldPaymentMethodPtr>>;
  using FiatCurrencies = std::optional<std::vector<mojom::MeldFiatCurrencyPtr>>;
  using CryptoCurrencies =
      std::optional<std::vector<mojom::MeldCryptoCurrencyPtr>>;
  using Countries = std::optional<std::vector<mojom::MeldCountryPtr>>;
  using CryptoWidget = mojom::MeldCryptoWidgetPtr;

  void SetInterceptor(
      const std::string& content,
      const net::HttpStatusCode http_status,
      std::optional<std::string> expected_request = std::nullopt) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, content, http_status,
         expected_request = std::move(expected_request)](
            const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          auto header =
              request.headers.GetHeader(brave_wallet::kMeldRpcVersionHeader);
          EXPECT_TRUE(header);
          EXPECT_FALSE(header->empty());

          if (expected_request) {
            EXPECT_THAT(request.request_body->elements()
                            ->at(0)
                            .As<network::DataElementBytes>()
                            .AsStringPiece(),
                        IsJson(*expected_request));
          }
          url_loader_factory_.AddResponse(request.url.spec(), content,
                                          http_status);
        }));
  }

  std::tuple<ServiceProviders, Errors> TestGetServiceProvider(
      const std::string& content,
      const net::HttpStatusCode http_status = net::HTTP_OK) {
    SetInterceptor(content, http_status);

    TestFuture<ServiceProviders, Errors> future;
    meld_integration_service_->GetServiceProviders(
        future.GetCallback<ServiceProviders, const Errors&>());
    return future.Take();
  }

  std::tuple<CryptoQuotes, Errors> TestGetCryptoQuotes(
      const std::string& content,
      const std::string& country,
      const std::string& from_asset,
      const std::string& to_asset,
      const double source_amount,
      const std::optional<std::string>& account,
      const std::optional<std::string>& payment_method,
      const std::string& expected_request,
      const net::HttpStatusCode http_status = net::HTTP_OK) {
    SetInterceptor(content, http_status, expected_request);

    TestFuture<CryptoQuotes, Errors> future;
    meld_integration_service_->GetCryptoQuotes(
        country, from_asset, to_asset, source_amount, account, payment_method,
        future.GetCallback<CryptoQuotes, const Errors&>());
    return future.Take();
  }

  std::tuple<PaymentMethods, Errors> TestGetPaymentMethods(
      const std::string& content,
      const std::string& country,
      const std::string& source_currency_code,
      const net::HttpStatusCode http_status = net::HTTP_OK) {
    SetInterceptor(content, http_status);

    TestFuture<PaymentMethods, Errors> future;
    meld_integration_service_->GetPaymentMethods(
        country, source_currency_code,
        future.GetCallback<PaymentMethods, const Errors&>());
    return future.Take();
  }

  std::tuple<FiatCurrencies, Errors> TestGetFiatCurrencies(
      const std::string& content,
      const net::HttpStatusCode http_status = net::HTTP_OK) {
    SetInterceptor(content, http_status);

    TestFuture<FiatCurrencies, Errors> future;
    meld_integration_service_->GetFiatCurrencies(
        future.GetCallback<FiatCurrencies, const Errors&>());
    return future.Take();
  }

  std::tuple<CryptoCurrencies, Errors> TestGetCryptoCurrencies(
      const std::string& content,
      const net::HttpStatusCode http_status = net::HTTP_OK) {
    SetInterceptor(content, http_status);

    TestFuture<CryptoCurrencies, Errors> future;
    meld_integration_service_->GetCryptoCurrencies(
        future.GetCallback<CryptoCurrencies, const Errors&>());
    return future.Take();
  }

  std::tuple<Countries, Errors> TestGetCountries(
      const std::string& content,
      const net::HttpStatusCode http_status = net::HTTP_OK) {
    SetInterceptor(content, http_status);

    TestFuture<Countries, Errors> future;
    meld_integration_service_->GetCountries(
        future.GetCallback<Countries, const Errors&>());
    return future.Take();
  }

  std::tuple<CryptoWidget, Errors> TestCryptoBuyWidgetCreate(
      const std::string& content,
      mojom::CryptoBuySessionDataPtr session_data,
      mojom::CryptoWidgetCustomerDataPtr customer_data,
      std::optional<std::string> expected_request,
      const net::HttpStatusCode http_status = net::HTTP_OK) {
    SetInterceptor(content, http_status, std::move(expected_request));

    TestFuture<CryptoWidget, Errors> future;
    meld_integration_service_->CryptoBuyWidgetCreate(
        std::move(session_data), std::move(customer_data),
        future.GetCallback<CryptoWidget, const Errors&>());
    return future.Take();
  }

  std::tuple<CryptoWidget, Errors> TestCryptoSellWidgetCreate(
      const std::string& content,
      mojom::CryptoSellSessionDataPtr session_data,
      mojom::CryptoWidgetCustomerDataPtr customer_data,
      std::optional<std::string> expected_request,
      const net::HttpStatusCode http_status = net::HTTP_OK) {
    SetInterceptor(content, http_status, std::move(expected_request));

    TestFuture<CryptoWidget, Errors> future;
    meld_integration_service_->CryptoSellWidgetCreate(
        std::move(session_data), std::move(customer_data),
        future.GetCallback<CryptoWidget, const Errors&>());
    return future.Take();
  }

  std::tuple<CryptoWidget, Errors> TestCryptoTransferWidgetCreate(
      const std::string& content,
      mojom::CryptoTransferSessionDataPtr session_data,
      mojom::CryptoWidgetCustomerDataPtr customer_data,
      std::optional<std::string> expected_request,
      const net::HttpStatusCode http_status = net::HTTP_OK) {
    SetInterceptor(content, http_status, std::move(expected_request));

    TestFuture<CryptoWidget, Errors> future;
    meld_integration_service_->CryptoTransferWidgetCreate(
        std::move(session_data), std::move(customer_data),
        future.GetCallback<CryptoWidget, const Errors&>());
    return future.Take();
  }

 protected:
  std::unique_ptr<MeldIntegrationService> meld_integration_service_;
  base::test::TaskEnvironment task_environment_;

 private:
  network::TestURLLoaderFactory url_loader_factory_;
};

TEST_F(MeldIntegrationServiceUnitTest, GetServiceProviders) {
  std::optional<std::vector<mojom::MeldServiceProviderPtr>> sps;
  Errors errors;

  const auto url = MeldIntegrationService::GetServiceProviderURL();
  EXPECT_EQ(url.path(), "/service-providers");
  EXPECT_THAT(net::UrlSearchParams(url).params(),
              ElementsAre(Pair("accountFilter", "false"),
                          Pair("statuses", "LIVE,RECENTLY_ADDED")));
  std::tie(sps, errors) = TestGetServiceProvider(R"([
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
  }])");
  EXPECT_FALSE(errors.has_value());
  ASSERT_TRUE(sps);

  std::vector<mojom::MeldServiceProviderPtr> expected;
  expected.push_back(mojom::MeldServiceProvider::New(
      "Banxa", "BANXA", "LIVE", "http://www.banxa.com",
      std::vector<std::string>{"CRYPTO_ONRAMP"},
      base::flat_map<std::string, std::string>{{"CRYPTO_ONRAMP", "LIVE"}},
      "https://images-serviceprovider.meld.io/BANXA/short_logo_dark.png",
      std::nullopt));
  expected.push_back(mojom::MeldServiceProvider::New(
      "Blockchain.com", "BLOCKCHAINDOTCOM", "LIVE",
      "https://www.blockchain.com", std::vector<std::string>{"CRYPTO_ONRAMP"},
      base::flat_map<std::string, std::string>{{"CRYPTO_ONRAMP", "LIVE"}},
      std::nullopt, std::nullopt));
  EXPECT_EQ(*sps, expected);

  std::tie(sps, errors) = TestGetServiceProvider(R"([{
    "status": "LIVE",
    "categories": [
      "CRYPTO_ONRAMP"
    ],
    "categoryStatuses": {
      "CRYPTO_ONRAMP": "LIVE"
    }
  }])");
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{
                         l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});

  std::tie(sps, errors) = TestGetServiceProvider(R"({
    "code": "UNAUTHORIZED",
    "message": "invalid profile or secret",
    "requestId": "315a",
    "timestamp": "2024-04-24T18:55:09.327818Z"
  })",
                                                 net::HTTP_UNAUTHORIZED);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{"invalid profile or secret"});

  std::tie(sps, errors) = TestGetServiceProvider("some wrone data");
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});

  std::tie(sps, errors) =
      TestGetServiceProvider("some wrone data", net::HTTP_REQUEST_TIMEOUT);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});

  std::tie(sps, errors) = TestGetServiceProvider(R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [
      "[sourceAmount] must not be null",
      "[sourceCurrencyCode] must not be blank"
    ],
    "requestId": "356d",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })",
                                                 net::HTTP_BAD_REQUEST);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>(
                         {"[sourceAmount] must not be null",
                          "[sourceCurrencyCode] must not be blank"}));
}

TEST_F(MeldIntegrationServiceUnitTest, GetCryptoQuotes) {
  std::optional<std::vector<mojom::MeldCryptoQuotePtr>> quotes;
  Errors errors;

  const std::string content = R"({
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
  })";

  constexpr char kRequest[] = R"({
    "countryCode": "US",
    "sourceCurrencyCode": "USD",
    "destinationCurrencyCode": "BTC",
    "sourceAmount": 50.0,
    "walletAddress": "btc account address",
    "paymentMethodType": "VISA"
  })";

  constexpr char kMinimalRequest[] = R"({
    "countryCode": "US",
    "sourceCurrencyCode": "USD",
    "destinationCurrencyCode": "BTC",
    "sourceAmount": 50.0
  })";

  auto check_parsed_content =
      [](const std::optional<std::vector<mojom::MeldCryptoQuotePtr>>& quotes,
         const std::optional<std::vector<std::string>>& errors) {
        EXPECT_FALSE(errors.has_value());
        ASSERT_TRUE(quotes);

        std::vector<mojom::MeldCryptoQuotePtr> expected;
        expected.push_back(mojom::MeldCryptoQuote::New(
            "CRYPTO_PURCHASE", "75286", "2", "USD", "50", "43.97", "43.97",
            "6.03", "3.53", "APPLE_PAY", "BTC", "0.00066413", std::nullopt,
            "20", "TRANSAK", "US"));
        EXPECT_EQ(*quotes, expected);
      };

  std::tie(quotes, errors) = TestGetCryptoQuotes(
      content, "US", "USD", "BTC", 50, "btc account address", "VISA", kRequest);
  check_parsed_content(quotes, errors);

  std::tie(quotes, errors) =
      TestGetCryptoQuotes(content, "US", "USD", "BTC", 50, std::nullopt,
                          std::nullopt, kMinimalRequest);
  check_parsed_content(quotes, errors);

  std::tie(quotes, errors) = TestGetCryptoQuotes(
      "some wrong data", "US", "USD", "BTC", 50, "btc account address", "VISA",
      kRequest, net::HTTP_REQUEST_TIMEOUT);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});

  std::tie(quotes, errors) = TestGetCryptoQuotes(
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
      "US", "USD", "BTC", 50, "btc account address", "VISA", kRequest);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{"error description"});
  EXPECT_FALSE(quotes);

  std::tie(quotes, errors) =
      TestGetCryptoQuotes(R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [
      "[sourceAmount] must not be null",
      "[sourceCurrencyCode] must not be blank"
    ],
    "requestId": "356dd2b40fa55037bfe9d190b6438f59",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })",
                          "US", "USD", "BTC", 50, "btc account address", "VISA",
                          kRequest, net::HTTP_BAD_REQUEST);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>(
                         {"[sourceAmount] must not be null",
                          "[sourceCurrencyCode] must not be blank"}));

  std::tie(quotes, errors) =
      TestGetCryptoQuotes(R"({
    "quotes": null,
    "message": null,
    "error": "No Valid Quote Combinations Found For Provided Quote Request."
  })",
                          "US", "USD", "BTC", 50, "btc account address", "VISA",
                          kRequest, net::HTTP_BAD_REQUEST);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors,
            std::vector<std::string>({"No Valid Quote Combinations Found For "
                                      "Provided Quote Request."}));
}

TEST_F(MeldIntegrationServiceUnitTest, GetPaymentMethods) {
  std::optional<std::vector<mojom::MeldPaymentMethodPtr>> payment_methods;
  Errors errors;

  const auto url = MeldIntegrationService::GetPaymentMethodsURL("US", "USD");
  EXPECT_EQ(url.path(), "/network-partner/supported/payment-methods");
  EXPECT_THAT(net::UrlSearchParams(url).params(),
              ElementsAre(Pair("category", "CRYPTO_ONRAMP"),  //
                          Pair("country", "US"),              //
                          Pair("currencyCode", "USD")));
  std::tie(payment_methods, errors) = TestGetPaymentMethods(R"([
  {
    "method": "ACH",
    "name": null,
    "type": "BANK_TRANSFER",
    "logo": {
      "dark": "https://images-paymentMethod.meld.io/ACH/logo_dark.png",
      "light": "https://images-paymentMethod.meld.io/ACH/logo_light.png"
    }
  }
  ])",
                                                            "US", "USD");
  EXPECT_FALSE(errors.has_value());
  ASSERT_TRUE(payment_methods);

  std::vector<mojom::MeldPaymentMethodPtr> expected;
  expected.push_back(mojom::MeldPaymentMethod::New(
      "ACH", std::nullopt, "BANK_TRANSFER",
      "https://images-paymentMethod.meld.io/ACH/logo_dark.png",
      "https://images-paymentMethod.meld.io/ACH/logo_light.png"));
  EXPECT_EQ(*payment_methods, expected);

  std::tie(payment_methods, errors) = TestGetPaymentMethods(R"([
  {
    "method": "ACH",
    "name": "ACH",
    "type": "BANK_TRANSFER",
    "logo": {
      "dark": null,
      "light": "https://images-paymentMethod.meld.io/ACH/logo_light.png"
    }
  }
  ])",
                                                            "US", "USD");
  EXPECT_FALSE(errors.has_value());
  ASSERT_TRUE(payment_methods);

  std::vector<mojom::MeldPaymentMethodPtr> expected_null_dark_logo;
  expected_null_dark_logo.push_back(mojom::MeldPaymentMethod::New(
      "ACH", "ACH", "BANK_TRANSFER", std::nullopt,
      "https://images-paymentMethod.meld.io/ACH/logo_light.png"));
  EXPECT_EQ(*payment_methods, expected_null_dark_logo);

  std::tie(payment_methods, errors) = TestGetPaymentMethods(
      "some wrong data", "US", "USD", net::HTTP_REQUEST_TIMEOUT);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});

  std::tie(payment_methods, errors) =
      TestGetPaymentMethods(R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [
      "[sourceAmount] must not be null",
      "[sourceCurrencyCode] must not be blank"
    ],
    "requestId": "356dd2b40fa55037bfe9d190b6438f59",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })",
                            "US", "USD", net::HTTP_BAD_REQUEST);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>(
                         {"[sourceAmount] must not be null",
                          "[sourceCurrencyCode] must not be blank"}));
}

TEST_F(MeldIntegrationServiceUnitTest, GetFiatCurrencies) {
  std::optional<std::vector<mojom::MeldFiatCurrencyPtr>> fiat_currencies;
  Errors errors;

  const auto url = MeldIntegrationService::GetFiatCurrenciesURL();
  EXPECT_EQ(url.path(), "/network-partner/supported/currencies");
  EXPECT_THAT(net::UrlSearchParams(url).params(),
              ElementsAre(Pair("category", "CRYPTO_ONRAMP"),  //
                          Pair("type", "FIAT")));

  std::tie(fiat_currencies, errors) = TestGetFiatCurrencies(R"({
  "currencies": [
    {
      "currencyCode": "AFN",
      "name": null,
      "symbol": "https://images-currency.meld.io/fiat/AFN/symbol.png"
    },
    {
      "currencyCode": "DZD",
      "name": "Algerian Dinar",
      "symbol": "https://images-currency.meld.io/fiat/DZD/symbol.png"
    }
  ]})");
  EXPECT_FALSE(errors.has_value());
  ASSERT_TRUE(fiat_currencies);

  std::vector<mojom::MeldFiatCurrencyPtr> expected;
  expected.push_back(mojom::MeldFiatCurrency::New(
      "AFN", std::nullopt,
      "https://images-currency.meld.io/fiat/AFN/symbol.png"));
  expected.push_back(mojom::MeldFiatCurrency::New(
      "DZD", "Algerian Dinar",
      "https://images-currency.meld.io/fiat/DZD/symbol.png"));
  EXPECT_EQ(*fiat_currencies, expected);

  std::tie(fiat_currencies, errors) =
      TestGetFiatCurrencies("some wrong data", net::HTTP_REQUEST_TIMEOUT);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});

  std::tie(fiat_currencies, errors) =
      TestGetFiatCurrencies(R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [
      "[sourceAmount] must not be null",
      "[sourceCurrencyCode] must not be blank"
    ],
    "requestId": "356dd2b40fa55037bfe9d190b6438f59",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })",
                            net::HTTP_BAD_REQUEST);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>(
                         {"[sourceAmount] must not be null",
                          "[sourceCurrencyCode] must not be blank"}));
}

TEST_F(MeldIntegrationServiceUnitTest, GetCryptoCurrencies) {
  std::optional<std::vector<mojom::MeldCryptoCurrencyPtr>> crypto_currencies;
  Errors errors;

  const auto url = MeldIntegrationService::GetCryptoCurrenciesURL();

  EXPECT_EQ(url.path(), "/network-partner/supported/currencies");
  EXPECT_THAT(net::UrlSearchParams(url).params(),
              ElementsAre(Pair("category", "CRYPTO_ONRAMP"),  //
                          Pair("type", "CRYPTO")));

  std::tie(crypto_currencies, errors) = TestGetCryptoCurrencies(R"({
  "currencies": [
    {
      "currencyCode": "USDT_POLYGON",
      "name": "Tether",
      "chainCode": "POLYGON",
      "chainId": "137",
      "contract": "0xe41d2489571d322189246dafa5ebde1f4699f498",
      "symbol": "https://images-currency.meld.io/crypto/USDT_POLYGON/symbol.png"
    },
    {
      "currencyCode": "00",
      "name": "00 Token",
      "chainCode": "ETH",
      "chainId": "1",
      "contract": "0x111111111117dc0aa78b770fa6a738034120c302",
      "symbol": "https://images-currency.meld.io/crypto/00/symbol.png"
    }
  ]})");
  EXPECT_FALSE(errors.has_value());
  ASSERT_TRUE(crypto_currencies);

  std::vector<mojom::MeldCryptoCurrencyPtr> expected;
  expected.push_back(mojom::MeldCryptoCurrency::New(
      mojom::CoinType::ETH, "0x89", "USDT", "USDT_POLYGON", "Tether", "POLYGON",
      "0xe41d2489571d322189246dafa5ebde1f4699f498",
      "https://images-currency.meld.io/crypto/USDT_POLYGON/symbol.png"));
  expected.push_back(mojom::MeldCryptoCurrency::New(
      mojom::CoinType::ETH, "0x1", "00", "00", "00 Token", "ETH",
      "0x111111111117dc0aa78b770fa6a738034120c302",
      "https://images-currency.meld.io/crypto/00/symbol.png"));
  EXPECT_EQ(*crypto_currencies, expected);

  std::tie(crypto_currencies, errors) =
      TestGetCryptoCurrencies("some wrong data", net::HTTP_REQUEST_TIMEOUT);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});

  std::tie(crypto_currencies, errors) =
      TestGetCryptoCurrencies(R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [
      "[sourceAmount] must not be null",
      "[sourceCurrencyCode] must not be blank"
    ],
    "requestId": "356dd2b40fa55037bfe9d190b6438f59",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })",
                              net::HTTP_BAD_REQUEST);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>(
                         {"[sourceAmount] must not be null",
                          "[sourceCurrencyCode] must not be blank"}));
}

TEST_F(MeldIntegrationServiceUnitTest, GetCountries) {
  std::optional<std::vector<mojom::MeldCountryPtr>> countries;
  Errors errors;

  const auto url = MeldIntegrationService::GetCountriesURL();
  EXPECT_EQ(url.path(), "/network-partner/supported/countries");
  EXPECT_THAT(net::UrlSearchParams(url).params(),
              ElementsAre(Pair("category", "CRYPTO_ONRAMP")));
  std::tie(countries, errors) = TestGetCountries(R"({
  "countries": [
    {
      "countryCode": "AF",
      "name": "Afghanistan"
    },
    {
      "countryCode": "AL",
      "name": "Albania"
    }
  ]})");
  EXPECT_FALSE(errors.has_value());
  ASSERT_TRUE(countries);

  std::vector<mojom::MeldCountryPtr> expected;
  expected.push_back(mojom::MeldCountry::New("AF", "Afghanistan"));
  expected.push_back(mojom::MeldCountry::New("AL", "Albania"));
  EXPECT_EQ(*countries, expected);

  std::tie(countries, errors) = TestGetCountries("some wrong data");
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});

  std::tie(countries, errors) =
      TestGetCountries("some wrong data", net::HTTP_REQUEST_TIMEOUT);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});

  std::tie(countries, errors) = TestGetCountries(R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [
      "[sourceAmount] must not be null",
      "[sourceCurrencyCode] must not be blank"
    ],
    "requestId": "356dd2b40fa55037bfe9d190b6438f59",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })",
                                                 net::HTTP_BAD_REQUEST);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>(
                         {"[sourceAmount] must not be null",
                          "[sourceCurrencyCode] must not be blank"}));
}

TEST_F(MeldIntegrationServiceUnitTest, CryptoBuyWidgetCreate) {
  mojom::MeldCryptoWidgetPtr crypto_widget;
  Errors errors;

  const auto session_data = mojom::CryptoBuySessionData::New(
      "US", "BTC", std::vector<std::string>{"cryptoCurrency"}, "BANK_TRANSFER",
      "https://sb.fluidmoney.xyz", "AKOYA", "100", "USD", "testWalletAddress",
      "walletTag");
  const auto customer_data = mojom::CryptoWidgetCustomerData::New(
      mojom::CustomerObject::New("customer@email"), "customer_id",
      "external_customer_id", "external_session_id");

  constexpr char kRequestWithCustomer[] = R"({
    "sessionType": "BUY",
    "sessionData": {
      "countryCode": "US",
      "destinationCurrencyCode": "BTC",
      "lockFields": ["cryptoCurrency"],
      "paymentMethodType": "BANK_TRANSFER",
      "redirectUrl": "https://sb.fluidmoney.xyz",
      "serviceProvider": "AKOYA",
      "sourceAmount": "100",
      "sourceCurrencyCode": "USD",
      "walletAddress": "testWalletAddress",
      "walletTag": "walletTag"
    },
    "customer": {"email": "customer@email"},
    "customerId": "customer_id",
    "externalCustomerId": "external_customer_id",
    "externalSessionId": "external_session_id"
  })";

  constexpr char kRequestWithoutCustomer[] = R"({
    "sessionType": "BUY",
    "sessionData": {
      "countryCode": "US",
      "destinationCurrencyCode": "BTC",
      "lockFields": ["cryptoCurrency"],
      "paymentMethodType": "BANK_TRANSFER",
      "redirectUrl": "https://sb.fluidmoney.xyz",
      "serviceProvider": "AKOYA",
      "sourceAmount": "100",
      "sourceCurrencyCode": "USD",
      "walletAddress": "testWalletAddress",
      "walletTag": "walletTag"
    }
  })";

  std::tie(crypto_widget, errors) = TestCryptoBuyWidgetCreate(
      R"({
  "id": "WXDpzmm2cNmtJWLDHgu1GT",
  "externalSessionId": "external_session_id",
  "externalCustomerId": "external_customer_id",
  "customerId": "WXEvEDzSgNedXWnJ55pwUJ",
  "widgetUrl": "https://meldcrypto.com?token=token-val",
  "token": "token-val"
})",
      session_data.Clone(), customer_data.Clone(), kRequestWithCustomer);
  EXPECT_FALSE(errors.has_value());
  ASSERT_TRUE(crypto_widget);
  EXPECT_EQ("WXDpzmm2cNmtJWLDHgu1GT", crypto_widget->id);
  EXPECT_EQ("external_customer_id", crypto_widget->external_customer_id);
  EXPECT_EQ("external_session_id", crypto_widget->external_session_id);
  EXPECT_EQ("WXEvEDzSgNedXWnJ55pwUJ", crypto_widget->customer_id);
  EXPECT_EQ("token-val", crypto_widget->token);
  EXPECT_EQ("https://meldcrypto.com?token=token-val",
            crypto_widget->widget_url);

  std::tie(crypto_widget, errors) = TestCryptoBuyWidgetCreate(
      R"({
  "externalSessionId": "external_session_id",
  "externalCustomerId": "external_customer_id",
  "customerId": "WXEvEDzSgNedXWnJ55pwUJ",
  "widgetUrl": "https://meldcrypto.com?token=token-val",
  "token": "token-val"
})",
      session_data.Clone(), nullptr, kRequestWithoutCustomer);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{
                         l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});

  std::tie(crypto_widget, errors) =
      TestCryptoBuyWidgetCreate("some wrong data", session_data.Clone(),
                                nullptr, kRequestWithoutCustomer);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});

  std::tie(crypto_widget, errors) = TestCryptoBuyWidgetCreate(
      "some wrong data", session_data.Clone(), nullptr, kRequestWithoutCustomer,
      net::HTTP_REQUEST_TIMEOUT);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});

  std::tie(crypto_widget, errors) =
      TestCryptoBuyWidgetCreate("", nullptr, nullptr, std::nullopt);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{l10n_util::GetStringUTF8(
                         IDS_WALLET_REQUEST_PROCESSING_ERROR)});

  std::tie(crypto_widget, errors) = TestCryptoBuyWidgetCreate(
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
      session_data.Clone(), nullptr, kRequestWithoutCustomer,
      net::HTTP_BAD_REQUEST);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>(
                         {"[sourceAmount] must not be null",
                          "[sourceCurrencyCode] must not be blank"}));
}

TEST_F(MeldIntegrationServiceUnitTest, CryptoSellWidgetCreate) {
  mojom::MeldCryptoWidgetPtr crypto_widget;
  Errors errors;

  const auto session_data = mojom::CryptoSellSessionData::New(
      "US", "BTC", std::vector<std::string>{"cryptoCurrency"}, "BANK_TRANSFER",
      "https://sb.fluidmoney.xyz", "AKOYA", "100", "USD", "testWalletAddress",
      "walletTag");
  auto session_data_without_wallet = session_data.Clone();
  session_data_without_wallet->wallet_address = std::nullopt;
  const auto customer_data = mojom::CryptoWidgetCustomerData::New(
      mojom::CustomerObject::New("customer@email"), "customer_id",
      "external_customer_id", "external_session_id");

  constexpr char kRequestWithCustomer[] = R"({
    "sessionType": "SELL",
    "sessionData": {
      "countryCode": "US",
      "destinationCurrencyCode": "BTC",
      "lockFields": ["cryptoCurrency"],
      "paymentMethodType": "BANK_TRANSFER",
      "redirectUrl": "https://sb.fluidmoney.xyz",
      "serviceProvider": "AKOYA",
      "sourceAmount": "100",
      "sourceCurrencyCode": "USD",
      "walletTag": "walletTag"
    },
    "customer": {"email": "customer@email"},
    "customerId": "customer_id",
    "externalCustomerId": "external_customer_id",
    "externalSessionId": "external_session_id"
  })";

  constexpr char kRequestWithoutCustomer[] = R"({
    "sessionType": "SELL",
    "sessionData": {
      "countryCode": "US",
      "destinationCurrencyCode": "BTC",
      "lockFields": ["cryptoCurrency"],
      "paymentMethodType": "BANK_TRANSFER",
      "redirectUrl": "https://sb.fluidmoney.xyz",
      "serviceProvider": "AKOYA",
      "sourceAmount": "100",
      "sourceCurrencyCode": "USD",
      "walletAddress": "testWalletAddress",
      "walletTag": "walletTag"
    }
  })";

  constexpr char kRequestWithExternalSessionId[] = R"({
    "sessionType": "SELL",
    "sessionData": {
      "countryCode": "US",
      "destinationCurrencyCode": "BTC",
      "lockFields": ["cryptoCurrency"],
      "paymentMethodType": "BANK_TRANSFER",
      "redirectUrl": "https://sb.fluidmoney.xyz",
      "serviceProvider": "AKOYA",
      "sourceAmount": "100",
      "sourceCurrencyCode": "USD",
      "walletAddress": "testWalletAddress",
      "walletTag": "walletTag"
    },
    "externalSessionId": "external_session_id"
  })";

  std::tie(crypto_widget, errors) = TestCryptoSellWidgetCreate(
      R"({
  "id": "WXDpzmm2cNmtJWLDHgu1GT",
  "externalSessionId": "external_session_id",
  "externalCustomerId": "external_customer_id",
  "customerId": "WXEvEDzSgNedXWnJ55pwUJ",
  "widgetUrl": "https://meldcrypto.com?token=token-val",
  "token": "token-val"
})",
      std::move(session_data_without_wallet), customer_data.Clone(),
      kRequestWithCustomer);
  EXPECT_FALSE(errors.has_value());
  EXPECT_EQ("WXDpzmm2cNmtJWLDHgu1GT", crypto_widget->id);
  EXPECT_EQ("external_customer_id", crypto_widget->external_customer_id);
  EXPECT_EQ("external_session_id", crypto_widget->external_session_id);
  EXPECT_EQ("WXEvEDzSgNedXWnJ55pwUJ", crypto_widget->customer_id);
  EXPECT_EQ("token-val", crypto_widget->token);
  EXPECT_EQ("https://meldcrypto.com?token=token-val",
            crypto_widget->widget_url);

  std::tie(crypto_widget, errors) = TestCryptoSellWidgetCreate(
      R"({
  "externalSessionId": "external_session_id",
  "externalCustomerId": "external_customer_id",
  "customerId": "WXEvEDzSgNedXWnJ55pwUJ",
  "widgetUrl": "https://meldcrypto.com?token=token-val",
  "token": "token-val"
})",
      session_data.Clone(), nullptr, kRequestWithoutCustomer);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{
                         l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});

  std::tie(crypto_widget, errors) =
      TestCryptoSellWidgetCreate("some wrong data", session_data.Clone(),
                                 nullptr, kRequestWithoutCustomer);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});

  std::tie(crypto_widget, errors) = TestCryptoSellWidgetCreate(
      "some wrong data", session_data.Clone(), nullptr, kRequestWithoutCustomer,
      net::HTTP_REQUEST_TIMEOUT);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});

  std::tie(crypto_widget, errors) =
      TestCryptoSellWidgetCreate("", nullptr, nullptr, std::nullopt);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{l10n_util::GetStringUTF8(
                         IDS_WALLET_REQUEST_PROCESSING_ERROR)});

  std::tie(crypto_widget, errors) = TestCryptoSellWidgetCreate(
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
      session_data.Clone(),
      mojom::CryptoWidgetCustomerData::New(nullptr, std::nullopt, std::nullopt,
                                           "external_session_id"),
      kRequestWithExternalSessionId, net::HTTP_BAD_REQUEST);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>(
                         {"[sourceAmount] must not be null",
                          "[sourceCurrencyCode] must not be blank"}));
}

TEST_F(MeldIntegrationServiceUnitTest, CryptoTransferWidgetCreate) {
  mojom::MeldCryptoWidgetPtr crypto_widget;
  Errors errors;

  const auto session_data = mojom::CryptoTransferSessionData::New(
      "US", "institution", std::vector<std::string>{"cryptoCurrency"},
      "https://sb.fluidmoney.xyz", "AKOYA", "1",
      std::vector<std::string>{"BTC", "ETH"}, std::nullopt, "walletTag");
  const auto customer_data = mojom::CryptoWidgetCustomerData::New(
      mojom::CustomerObject::New("customer@email"), "customer_id",
      "external_customer_id", "external_session_id");

  constexpr char kRequestWithCustomer[] = R"({
    "sessionType": "TRANSFER",
    "sessionData": {
      "countryCode": "US",
      "institutionId": "institution",
      "lockFields": ["cryptoCurrency"],
      "redirectUrl": "https://sb.fluidmoney.xyz",
      "serviceProvider": "AKOYA",
      "sourceAmount": "1",
      "sourceCurrencyCodes": ["BTC", "ETH"],
      "walletTag": "walletTag"
    },
    "customer": {"email": "customer@email"},
    "customerId": "customer_id",
    "externalCustomerId": "external_customer_id",
    "externalSessionId": "external_session_id"
  })";

  constexpr char kRequestWithoutCustomer[] = R"({
    "sessionType": "TRANSFER",
    "sessionData": {
      "countryCode": "US",
      "institutionId": "institution",
      "lockFields": ["cryptoCurrency"],
      "redirectUrl": "https://sb.fluidmoney.xyz",
      "serviceProvider": "AKOYA",
      "sourceAmount": "1",
      "sourceCurrencyCodes": ["BTC", "ETH"],
      "walletTag": "walletTag"
    }
  })";

  std::tie(crypto_widget, errors) = TestCryptoTransferWidgetCreate(
      R"({
  "id": "WXDpzmm2cNmtJWLDHgu1GT",
  "externalSessionId": "external_session_id",
  "externalCustomerId": "external_customer_id",
  "customerId": "WXEvEDzSgNedXWnJ55pwUJ",
  "widgetUrl": "https://meldcrypto.com?token=token-val",
  "token": "token-val"
})",
      session_data.Clone(), customer_data.Clone(), kRequestWithCustomer);
  EXPECT_FALSE(errors.has_value());
  EXPECT_EQ("WXDpzmm2cNmtJWLDHgu1GT", crypto_widget->id);
  EXPECT_EQ("external_customer_id", crypto_widget->external_customer_id);
  EXPECT_EQ("external_session_id", crypto_widget->external_session_id);
  EXPECT_EQ("WXEvEDzSgNedXWnJ55pwUJ", crypto_widget->customer_id);
  EXPECT_EQ("token-val", crypto_widget->token);
  EXPECT_EQ("https://meldcrypto.com?token=token-val",
            crypto_widget->widget_url);

  std::tie(crypto_widget, errors) = TestCryptoTransferWidgetCreate(
      R"({
  "externalSessionId": "external_session_id",
  "externalCustomerId": "external_customer_id",
  "customerId": "WXEvEDzSgNedXWnJ55pwUJ",
  "widgetUrl": "https://meldcrypto.com?token=token-val",
  "token": "token-val"
})",
      session_data.Clone(), nullptr, kRequestWithoutCustomer);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{
                         l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)});

  std::tie(crypto_widget, errors) =
      TestCryptoTransferWidgetCreate("some wrong data", session_data.Clone(),
                                     nullptr, kRequestWithoutCustomer);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});

  std::tie(crypto_widget, errors) = TestCryptoTransferWidgetCreate(
      "some wrong data", session_data.Clone(), nullptr, kRequestWithoutCustomer,
      net::HTTP_REQUEST_TIMEOUT);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});

  std::tie(crypto_widget, errors) =
      TestCryptoTransferWidgetCreate("", nullptr, nullptr, std::nullopt);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>{l10n_util::GetStringUTF8(
                         IDS_WALLET_REQUEST_PROCESSING_ERROR)});

  std::tie(crypto_widget, errors) = TestCryptoTransferWidgetCreate(
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
      session_data.Clone(), customer_data.Clone(), kRequestWithCustomer,
      net::HTTP_BAD_REQUEST);
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(*errors, std::vector<std::string>(
                         {"[sourceAmount] must not be null",
                          "[sourceCurrencyCode] must not be blank"}));
}

}  // namespace brave_wallet
