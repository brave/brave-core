/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/meld_integration_response_parser.h"

#include <cstddef>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/common/features.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJson;
using testing::ElementsAreArray;

namespace brave_wallet {

TEST(MeldIntegrationResponseParserUnitTest, Parse_ServiceProvider) {
  std::string json(R"([
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
      "lightShort": "https://images-serviceprovider.meld.io/BANXA/short_logo_light.png"
    }
  }])");

  auto service_providers = ParseServiceProviders(ParseJson(json));
  ASSERT_TRUE(service_providers);

  std::vector<mojom::MeldServiceProviderPtr> expected;
  expected.push_back(mojom::MeldServiceProvider::New(
      "Banxa", "BANXA", "LIVE", "http://www.banxa.com",
      std::vector<std::string>{"CRYPTO_ONRAMP"},
      base::flat_map<std::string, std::string>{{"CRYPTO_ONRAMP", "LIVE"}},
      "https://images-serviceprovider.meld.io/BANXA/short_logo_dark.png",
      "https://images-serviceprovider.meld.io/BANXA/short_logo_light.png"));
  EXPECT_EQ(*service_providers, expected);
  std::string json_null_logos(R"([
  {
    "serviceProvider": "BANXA",
    "name": "Banxa",
    "status": "LIVE",
    "categories": [],
    "categoryStatuses": {
      "CRYPTO_ONRAMP": "LIVE"
    },
    "websiteUrl": "http://www.banxa.com"
  }])");
  service_providers = ParseServiceProviders(ParseJson(json_null_logos));
  ASSERT_TRUE(service_providers);

  std::vector<mojom::MeldServiceProviderPtr> expected_null_logos;
  expected_null_logos.push_back(mojom::MeldServiceProvider::New(
      "Banxa", "BANXA", "LIVE", "http://www.banxa.com",
      std::vector<std::string>{},
      base::flat_map<std::string, std::string>{{"CRYPTO_ONRAMP", "LIVE"}},
      std::nullopt, std::nullopt));
  EXPECT_EQ(*service_providers, expected_null_logos);

  EXPECT_FALSE(ParseServiceProviders(ParseJson(R"([
  {
    "name": "Banxa",
    "status": "LIVE",
    "categories": [],
    "categoryStatuses": {
      "CRYPTO_ONRAMP": "LIVE"
    },
    "websiteUrl": "http://www.banxa.com"
  }])")));

  EXPECT_FALSE(ParseServiceProviders(ParseJson(R"([
  {
    "serviceProvider": "BANXA",
    "name": "Banxa",
    "categories": [],
    "categoryStatuses": {
      "CRYPTO_ONRAMP": "LIVE"
    },
    "websiteUrl": "http://www.banxa.com"
  }])")));

  EXPECT_FALSE(ParseServiceProviders(ParseJson(R"([
  {
    "serviceProvider": "BANXA",
    "name": "Banxa",
    "status": "LIVE",
    "categoryStatuses": {
      "CRYPTO_ONRAMP": "LIVE"
    },
    "websiteUrl": "http://www.banxa.com"
  }])")));
  EXPECT_FALSE(ParseServiceProviders(ParseJson(R"([
  {
    "serviceProvider": "BANXA",
    "name": "Banxa",
    "status": "LIVE",
    "categories": [],
    "websiteUrl": "http://www.banxa.com"
  }])")));

  // Invalid json
  EXPECT_FALSE(ParseServiceProviders(base::Value()));

  // Valid json, missing required field
  json = (R"({})");
  EXPECT_FALSE(ParseServiceProviders(ParseJson(json)));
}

TEST(MeldIntegrationResponseParserUnitTest, Parse_MeldErrorResponse) {
  std::string json(R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [
      "[sourceAmount] must not be null",
      "[sourceCurrencyCode] must not be blank"
    ],
    "requestId": "356dd2b40fa55037bfe9d190b6438f59",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })");

  auto errors = ParseMeldErrorResponse(ParseJson(json));
  EXPECT_TRUE(errors.has_value());
  EXPECT_EQ(errors->size(), static_cast<size_t>(2));

  errors = ParseMeldErrorResponse(base::Value());
  EXPECT_FALSE(errors.has_value());

  errors = ParseMeldErrorResponse(ParseJson((R"({})")));
  EXPECT_FALSE(errors.has_value());

  std::string json_only_msg(R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [],
    "requestId": "356dd2b40fa55037bfe9d190b6438f59",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })");
  errors = ParseMeldErrorResponse(ParseJson(json_only_msg));
  EXPECT_TRUE(errors);
  EXPECT_EQ(errors->size(), static_cast<size_t>(1));

  // Malformed success responses are not Meld errors.
  EXPECT_FALSE(ParseMeldErrorResponse(
      ParseJson(R"([{"countryCode": "AL", "name": "Albania"}])")));
  EXPECT_FALSE(ParseMeldErrorResponse(
      ParseJson(R"({"countries": [{"name": "Albania"}]})")));
  EXPECT_FALSE(ParseMeldErrorResponse(
      ParseJson(R"({"currencies": [{"name": "Afghani"}]})")));
  EXPECT_FALSE(ParseMeldErrorResponse(
      ParseJson(R"({"method": "ACH", "type": "BANK_TRANSFER"})")));
}

TEST(MeldIntegrationResponseParserUnitTest, Parse_CryptoQuotes) {
  std::string json(R"({"quotes": [{
      "transactionType": "CRYPTO_PURCHASE",
      "sourceAmount": "50",
      "sourceAmountWithoutFees": "43.97",
      "fiatAmountWithoutFees": "43.97",
      "destinationAmountWithoutFees": "11.01",
      "sourceCurrencyCode": "USD",
      "countryCode": "US",
      "totalFee": "6.03",
      "networkFee": "3.53",
      "transactionFee": "2",
      "destinationAmount": "0.00066413",
      "destinationCurrencyCode": "BTC",
      "exchangeRate": "75286",
      "paymentMethodType": "APPLE_PAY",
      "customerScore": "20",
      "serviceProvider": "TRANSAK"}]})");

  auto quotes_result = ParseCryptoQuotes(ParseJson(json));
  ASSERT_TRUE(quotes_result.has_value());

  std::vector<mojom::MeldCryptoQuotePtr> expected;
  expected.push_back(mojom::MeldCryptoQuote::New(
      "CRYPTO_PURCHASE", "75286", "2", "USD", "50", "43.97", "43.97", "6.03",
      "3.53", "APPLE_PAY", "BTC", "0.00066413", "11.01", "20", "TRANSAK",
      "US"));
  EXPECT_EQ(quotes_result.value(), expected);

  std::string json_null_quotes(R"({
  "error": "No Valid Quote Combinations Found For Provided Quote Request."})");

  quotes_result = ParseCryptoQuotes(ParseJson(json_null_quotes));
  EXPECT_FALSE(quotes_result.has_value());
  EXPECT_EQ(quotes_result.error(),
            "No Valid Quote Combinations Found For Provided Quote Request.");

  quotes_result = ParseCryptoQuotes(base::Value());
  EXPECT_FALSE(quotes_result.has_value());

  json = R"({})";
  quotes_result = ParseCryptoQuotes(ParseJson(json));
  EXPECT_TRUE(quotes_result.has_value());
  EXPECT_TRUE(quotes_result.value().empty());
}

TEST(MeldIntegrationResponseParserUnitTest, Parse_PaymentMethods) {
  std::string json(R"([
  {
    "method": "ACH",
    "name": "ACH",
    "type": "BANK_TRANSFER",
    "logo": {
      "dark": "https://images-paymentMethod.meld.io/ACH/logo_dark.png",
      "light": "https://images-paymentMethod.meld.io/ACH/logo_light.png"
    }
  }
  ])");

  auto payment_methods = ParsePaymentMethods(ParseJson(json));
  ASSERT_TRUE(payment_methods);

  std::vector<mojom::MeldPaymentMethodPtr> expected;
  expected.push_back(mojom::MeldPaymentMethod::New(
      "ACH", "ACH", "BANK_TRANSFER",
      "https://images-paymentMethod.meld.io/ACH/logo_dark.png",
      "https://images-paymentMethod.meld.io/ACH/logo_light.png"));
  EXPECT_EQ(*payment_methods, expected);
  std::string json_null_dark_logo(R"([
  {
    "method": "ACH",
    "name": "ACH",
    "type": "BANK_TRANSFER",
    "logo": {
      "light": "https://images-paymentMethod.meld.io/ACH/logo_light.png"
    }
  }
  ])");
  payment_methods = ParsePaymentMethods(ParseJson(json_null_dark_logo));
  ASSERT_TRUE(payment_methods);

  std::vector<mojom::MeldPaymentMethodPtr> expected_null_dark_logo;
  expected_null_dark_logo.push_back(mojom::MeldPaymentMethod::New(
      "ACH", "ACH", "BANK_TRANSFER", std::nullopt,
      "https://images-paymentMethod.meld.io/ACH/logo_light.png"));
  EXPECT_EQ(*payment_methods, expected_null_dark_logo);

  // `method` is a required field.
  EXPECT_FALSE(ParsePaymentMethods(ParseJson(R"([
  {
    "name": "ACH",
    "type": "BANK_TRANSFER",
    "logo": {
      "light": "https://images-paymentMethod.meld.io/ACH/logo_light.png"
    }
  }
  ])")));

  EXPECT_FALSE(ParsePaymentMethods(base::Value()));

  json = (R"({})");
  EXPECT_FALSE(ParsePaymentMethods(ParseJson(json)));
}

TEST(MeldIntegrationResponseParserUnitTest, Parse_FiatCurrencies) {
  std::string json(R"({
  "currencies": [
    {
      "currencyCode": "AFN",
      "name": "Afghani",
      "symbol": "https://images-currency.meld.io/fiat/AFN/symbol.png"
    }
  ]})");

  auto fiat_currencies = ParseFiatCurrencies(ParseJson(json));
  ASSERT_TRUE(fiat_currencies);

  std::vector<mojom::MeldFiatCurrencyPtr> expected;
  expected.push_back(mojom::MeldFiatCurrency::New(
      "AFN", "Afghani", "https://images-currency.meld.io/fiat/AFN/symbol.png"));
  EXPECT_EQ(*fiat_currencies, expected);

  // `currencyCode` is a required field.
  EXPECT_FALSE(ParseFiatCurrencies(ParseJson(R"({
  "currencies": [
    {
      "name": "Afghani",
      "symbol": "https://images-currency.meld.io/fiat/AFN/symbol.png"
    }
  ]})")));

  // A bare list is not a valid response anymore.
  EXPECT_FALSE(ParseFiatCurrencies(ParseJson(R"([
  {
    "currencyCode": "AFN",
    "name": "Afghani"
  }])")));

  EXPECT_FALSE(ParseFiatCurrencies(base::Value()));

  json = (R"({})");
  EXPECT_FALSE(ParseFiatCurrencies(ParseJson(json)));
}

TEST(MeldIntegrationResponseParserUnitTest, Parse_CryptoCurrencies) {
  std::string json(R"({
  "currencies": [
    {
      "currencyCode": "USDT_BSC",
      "name": "#REF!",
      "chainCode": "BSC",
      "chainId": "56",
      "contract": "0xe41d2489571d322189246dafa5ebde1f4699f498",
      "symbol": "https://images-currency.meld.io/crypto/USDT_BSC/symbol.png"
    }
  ]})");
  auto crypto_currencies = ParseCryptoCurrencies(ParseJson(json));
  ASSERT_TRUE(crypto_currencies);

  std::vector<mojom::MeldCryptoCurrencyPtr> expected;
  expected.push_back(mojom::MeldCryptoCurrency::New(
      mojom::CoinType::ETH, mojom::kBnbSmartChainMainnetChainId, "USDT",
      "USDT_BSC", "#REF!", "BSC", "0xe41d2489571d322189246dafa5ebde1f4699f498",
      "https://images-currency.meld.io/crypto/USDT_BSC/symbol.png"));
  EXPECT_EQ(*crypto_currencies, expected);

  // A missing required field fails the whole response.
  EXPECT_FALSE(ParseCryptoCurrencies(ParseJson(R"({
  "currencies": [
    {"currencyCode": "USDT", "name": "Tether", "symbol": ""}
  ]})")));

  // `currencyCode` is a required field.
  EXPECT_FALSE(ParseCryptoCurrencies(ParseJson(R"({
  "currencies": [
    {
      "name": "#REF!",
      "chainCode": "KCC",
      "chainId": "56",
      "contract": "0xe41d2489571d322189246dafa5ebde1f4699f498",
      "symbol": "https://images-currency.meld.io/crypto/USDT_KCC/symbol.png"
    }
  ]})")));

  // A bare list is not a valid response anymore.
  EXPECT_FALSE(ParseCryptoCurrencies(ParseJson(R"([
  {
    "currencyCode": "USDT_BSC",
    "name": "#REF!",
    "chainCode": "BSC",
    "symbol": ""
  }])")));

  EXPECT_FALSE(ParseCryptoCurrencies(base::Value()));

  json = (R"({})");
  EXPECT_FALSE(ParseCryptoCurrencies(ParseJson(json)));
}

TEST(MeldIntegrationResponseParserUnitTest, Parse_CryptoCurrencies_Chains) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kBraveWalletPolkadotFeature);

  // Networks are resolved by chainCode. Meld's chainId must match for EVM
  // chains and is ignored for others. Unsupported chain codes are dropped.
  std::string json = R"({"currencies": [
  {"currencyCode": "BTC", "chainCode": "BTC"},
  {"currencyCode": "FIL", "chainCode": "FIL"},
  {"currencyCode": "ZEC", "chainCode": "ZEC"},
  {"currencyCode": "SOL", "chainCode": "SOLANA", "chainId": "101"},
  {"currencyCode": "ADA", "chainCode": "ADA"},
  {"currencyCode": "DOT", "chainCode": "DOT"},
  {"currencyCode": "USDT_DOT", "chainCode": "DOT"},
  {"currencyCode": "ETH", "chainCode": "ETH", "chainId": "1"},
  {"currencyCode": "USDC_BASE", "chainCode": "BASE", "chainId": "8453"},
  {"currencyCode": "AURORA", "chainCode": "AURORA", "chainId": "1313161554"},
  {"currencyCode": "POL", "chainCode": "POLYGON"},
  {"currencyCode": "USDT_ARB", "chainCode": "ARBITRUM", "chainId": "1"},
  {"currencyCode": "USDT_OP", "chainCode": "OPTIMISM", "chainId": "abc"},
  {"currencyCode": "USDT_KCC", "chainCode": "KCC", "chainId": "321"}
  ]})";
  auto json_value = ParseJson(json);
  for (auto& item : *json_value.GetDict().FindList("currencies")) {
    // Required fields not under test.
    auto& dict = item.GetDict();
    dict.Set("name", "");
    dict.Set("symbol", "");
  }
  auto crypto_currencies = ParseCryptoCurrencies(json_value);
  ASSERT_TRUE(crypto_currencies);
  using ChainTuple = std::tuple<std::string, mojom::CoinType, std::string>;
  std::vector<ChainTuple> chains;
  for (const auto& item : *crypto_currencies) {
    chains.emplace_back(item->currency_code, item->coin, item->chain_id);
  }
  EXPECT_THAT(
      chains,
      ElementsAreArray<ChainTuple>({
          {"BTC", mojom::CoinType::BTC, mojom::kBitcoinMainnet},
          {"FIL", mojom::CoinType::FIL, mojom::kFilecoinMainnet},
          {"ZEC", mojom::CoinType::ZEC, mojom::kZCashMainnet},
          {"SOL", mojom::CoinType::SOL, mojom::kSolanaMainnet},
          {"ADA", mojom::CoinType::ADA, mojom::kCardanoMainnet},
          {"DOT", mojom::CoinType::DOT, mojom::kPolkadotMainnetAssetHub},
          {"USDT_DOT", mojom::CoinType::DOT, mojom::kPolkadotMainnetAssetHub},
          {"ETH", mojom::CoinType::ETH, mojom::kMainnetChainId},
          {"USDC_BASE", mojom::CoinType::ETH, mojom::kBaseMainnetChainId},
          {"AURORA", mojom::CoinType::ETH, mojom::kAuroraMainnetChainId},
          {"POL", mojom::CoinType::ETH, mojom::kPolygonMainnetChainId},
      }));
}

TEST(MeldIntegrationResponseParserUnitTest, Parse_CryptoCurrencies_Polkadot) {
  constexpr char kJson[] = R"({
  "currencies": [
    {
      "currencyCode": "DOT",
      "name": "Polkadot",
      "chainCode": "DOT",
      "chainId": "0",
      "symbol": "https://images-currency.meld.io/crypto/DOT/symbol.png"
    }
  ]})";

  {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeature(features::kBraveWalletPolkadotFeature);

    auto crypto_currencies = ParseCryptoCurrencies(ParseJson(kJson));
    ASSERT_TRUE(crypto_currencies);
    ASSERT_EQ(crypto_currencies->size(), 1u);
    EXPECT_EQ(crypto_currencies->at(0)->currency_code, "DOT");
    EXPECT_EQ(crypto_currencies->at(0)->coin, mojom::CoinType::DOT);
    EXPECT_EQ(crypto_currencies->at(0)->chain_id,
              mojom::kPolkadotMainnetAssetHub);
  }

  {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndDisableFeature(features::kBraveWalletPolkadotFeature);

    auto crypto_currencies = ParseCryptoCurrencies(ParseJson(kJson));
    ASSERT_TRUE(crypto_currencies);
    EXPECT_TRUE(crypto_currencies->empty());
  }
}

TEST(MeldIntegrationResponseParserUnitTest, Parse_Countries) {
  std::string json(R"({
  "countries": [
    {
      "countryCode": "AF",
      "name": "Afghanistan"
    }
  ]})");
  auto countries = ParseCountries(ParseJson(json));
  ASSERT_TRUE(countries);

  std::vector<mojom::MeldCountryPtr> expected;
  expected.push_back(mojom::MeldCountry::New("AF", "Afghanistan"));
  EXPECT_EQ(*countries, expected);

  // `countryCode` is a required field.
  EXPECT_FALSE(ParseCountries(ParseJson(R"({
  "countries": [
    {
      "name": "Afghanistan"
    }
  ]})")));

  // A bare list is not a valid response anymore.
  EXPECT_FALSE(ParseCountries(ParseJson(R"([
  {
    "countryCode": "AF",
    "name": "Afghanistan"
  }])")));

  EXPECT_FALSE(ParseCountries(base::Value()));

  json = (R"({})");
  EXPECT_FALSE(ParseCountries(ParseJson(json)));
}

TEST(MeldIntegrationResponseParserUnitTest, Parse_CryptoWidgetCreate) {
  std::string json(R"({
    "id": "WXDpzmm2cNmtJWLDHgu1GT",
    "externalSessionId": "test_session_id",
    "externalCustomerId": "test_session_customer_id",
    "customerId": "WXEvEDzSgNedXWnJ55pwUJ",
    "widgetUrl": "https://meldcrypto.com?token=tknvalue",
    "token": "tknvalue"
  })");
  auto result = ParseCryptoWidgetCreate(ParseJson(json));
  EXPECT_TRUE(result);
  EXPECT_EQ("WXDpzmm2cNmtJWLDHgu1GT", result->id);
  EXPECT_EQ("test_session_id", result->external_session_id);
  EXPECT_EQ("test_session_customer_id", result->external_customer_id);
  EXPECT_EQ("WXEvEDzSgNedXWnJ55pwUJ", result->customer_id);
  EXPECT_EQ("https://meldcrypto.com?token=tknvalue", result->widget_url);
  EXPECT_EQ("tknvalue", result->token);

  json = R"({
    "id": "WXDpzmm2cNmtJWLDHgu1GT",
    "customerId": "WXEvEDzSgNedXWnJ55pwUJ",
    "widgetUrl": "https://meldcrypto.com?token=tknvalue",
    "token": "tknvalue"
  })";
  result = ParseCryptoWidgetCreate(ParseJson(json));
  EXPECT_TRUE(result);
  EXPECT_EQ("WXDpzmm2cNmtJWLDHgu1GT", result->id);
  EXPECT_FALSE(result->external_session_id);
  EXPECT_FALSE(result->external_customer_id);
  EXPECT_EQ("WXEvEDzSgNedXWnJ55pwUJ", result->customer_id);
  EXPECT_EQ("https://meldcrypto.com?token=tknvalue", result->widget_url);
  EXPECT_EQ("tknvalue", result->token);

  // id is mandatory field
  json = R"({
    "externalSessionId": "test_session_id",
    "externalCustomerId": "test_session_customer_id",
    "customerId": "WXEvEDzSgNedXWnJ55pwUJ",
    "widgetUrl": "https://meldcrypto.com?token=tknvalue",
    "token": "tknvalue"
  })";
  EXPECT_FALSE(ParseCryptoWidgetCreate(ParseJson(json)));

  // customerId is mandatory field
  json = R"({
    "id": "WXDpzmm2cNmtJWLDHgu1GT",
    "externalSessionId": "test_session_id",
    "externalCustomerId": "test_session_customer_id",
    "customerId": "",
    "widgetUrl": "https://meldcrypto.com?token=tknvalue",
    "token": "tknvalue"
  })";
  EXPECT_FALSE(ParseCryptoWidgetCreate(ParseJson(json)));

  // widgetUrl is valid HTTP(S) URL
  json = R"({
    "id": "WXDpzmm2cNmtJWLDHgu1GT",
    "externalSessionId": "test_session_id",
    "externalCustomerId": "test_session_customer_id",
    "customerId": "WXEvEDzSgNedXWnJ55pwUJ",
    "widgetUrl": "wrong_url",
    "token": "tknvalue"
  })";
  EXPECT_FALSE(ParseCryptoWidgetCreate(ParseJson(json)));

  // widgetUrl is valid HTTP(S) URL
  json = R"({
    "id": "WXDpzmm2cNmtJWLDHgu1GT",
    "externalSessionId": "test_session_id",
    "externalCustomerId": "test_session_customer_id",
    "customerId": "WXEvEDzSgNedXWnJ55pwUJ",
    "widgetUrl": "ftp://meldcrypto.com?token=tknvalue",
    "token": "tknvalue"
  })";
  EXPECT_FALSE(ParseCryptoWidgetCreate(ParseJson(json)));

  // widgetUrl is mandatory field
  json = R"({
    "id": "WXDpzmm2cNmtJWLDHgu1GT",
    "externalSessionId": "test_session_id",
    "externalCustomerId": "test_session_customer_id",
    "customerId": "WXEvEDzSgNedXWnJ55pwUJ",
    "token": "tknvalue"
  })";
  EXPECT_FALSE(ParseCryptoWidgetCreate(ParseJson(json)));

  // token is mandatory field
  json = R"({
    "id": "WXDpzmm2cNmtJWLDHgu1GT",
    "externalSessionId": "test_session_id",
    "externalCustomerId": "test_session_customer_id",
    "customerId": "WXEvEDzSgNedXWnJ55pwUJ",
    "widgetUrl": "https://meldcrypto.com?token=tknvalue"
  })";
  EXPECT_FALSE(ParseCryptoWidgetCreate(ParseJson(json)));

  json = (R"({})");
  EXPECT_FALSE(ParseCryptoWidgetCreate(ParseJson(json)));
}
}  // namespace brave_wallet
