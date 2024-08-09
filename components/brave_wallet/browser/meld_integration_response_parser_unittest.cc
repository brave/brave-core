/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/meld_integration_response_parser.h"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/string_util.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJson;

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
      "dark": "https://images-serviceprovider.meld.io/BANXA/logo_dark.png",
      "light": "https://images-serviceprovider.meld.io/BANXA/logo_light.png",
      "darkShort": "https://images-serviceprovider.meld.io/BANXA/short_logo_dark.png",
      "lightShort": "https://images-serviceprovider.meld.io/BANXA/short_logo_light.png"
    }
  }])");

  auto service_providers = ParseServiceProviders(ParseJson(json));
  EXPECT_TRUE(service_providers);
  EXPECT_EQ(base::ranges::count_if(
                *service_providers,
                [](const auto& item) {
                  return item->name == "Banxa" &&
                         item->service_provider == "BANXA" &&
                         item->status == "LIVE" &&
                         item->web_site_url == "http://www.banxa.com" &&
                         !item->categories.empty() &&
                         item->categories[0] == "CRYPTO_ONRAMP" &&
                         !item->category_statuses.empty() &&
                         item->category_statuses.contains("CRYPTO_ONRAMP") &&
                         item->category_statuses["CRYPTO_ONRAMP"] == "LIVE" &&
                         item->logo_images &&
                         item->logo_images->dark_url ==
                             "https://images-serviceprovider.meld.io/BANXA/"
                             "logo_dark.png" &&
                         item->logo_images->dark_short_url ==
                             "https://images-serviceprovider.meld.io/BANXA/"
                             "short_logo_dark.png" &&
                         item->logo_images->light_url ==
                             "https://images-serviceprovider.meld.io/BANXA/"
                             "logo_light.png" &&
                         item->logo_images->light_short_url ==
                             "https://images-serviceprovider.meld.io/BANXA/"
                             "short_logo_light.png";
                }),
            1);
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
  EXPECT_TRUE(service_providers);
  EXPECT_EQ(base::ranges::count_if(
                *service_providers,
                [](const auto& item) {
                  return item->name == "Banxa" &&
                         item->service_provider == "BANXA" &&
                         item->status == "LIVE" &&
                         item->web_site_url == "http://www.banxa.com" &&
                         item->categories.empty() &&
                         !item->category_statuses.empty() &&
                         item->category_statuses.contains("CRYPTO_ONRAMP") &&
                         item->category_statuses["CRYPTO_ONRAMP"] == "LIVE" &&
                         !item->logo_images;
                }),
            1);

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
  EXPECT_TRUE(quotes_result.has_value());
  EXPECT_EQ(base::ranges::count_if(
                quotes_result.value(),
                [](const auto& item) {
                  return item->transaction_type == "CRYPTO_PURCHASE" &&
                         item->source_amount == "50" &&
                         item->source_amount_without_fee == "43.97" &&
                         item->fiat_amount_without_fees == "43.97" &&
                         item->destination_amount_without_fees == "11.01" &&
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
    "paymentMethod": "ACH",
    "name": "ACH",
    "paymentType": "BANK_TRANSFER",
    "logos": {
      "dark": "https://images-paymentMethod.meld.io/ACH/logo_dark.png",
      "light": "https://images-paymentMethod.meld.io/ACH/logo_light.png"
    }
  }
  ])");

  auto payment_methods = ParsePaymentMethods(ParseJson(json));
  EXPECT_TRUE(payment_methods);
  EXPECT_EQ(base::ranges::count_if(
                *payment_methods,
                [](const auto& item) {
                  return item->payment_method == "ACH" && item->name == "ACH" &&
                         item->payment_type == "BANK_TRANSFER" &&
                         item->logo_images &&
                         !item->logo_images->dark_short_url &&
                         !item->logo_images->light_short_url &&
                         *(item->logo_images->dark_url) ==
                             "https://images-paymentMethod.meld.io/ACH/"
                             "logo_dark.png" &&
                         *(item->logo_images->light_url) ==
                             "https://images-paymentMethod.meld.io/ACH/"
                             "logo_light.png";
                }),
            1);
  std::string json_null_dark_logo(R"([
  {
    "paymentMethod": "ACH",
    "name": "ACH",
    "paymentType": "BANK_TRANSFER",
    "logos": {
      "light": "https://images-paymentMethod.meld.io/ACH/logo_light.png"
    }
  }
  ])");
  payment_methods = ParsePaymentMethods(ParseJson(json_null_dark_logo));
  EXPECT_TRUE(payment_methods);
  EXPECT_EQ(base::ranges::count_if(
                *payment_methods,
                [](const auto& item) {
                  return item->payment_method == "ACH" && item->name == "ACH" &&
                         item->payment_type == "BANK_TRANSFER" &&
                         item->logo_images &&
                         !item->logo_images->dark_short_url &&
                         !item->logo_images->light_short_url &&
                         !item->logo_images->dark_url &&
                         *(item->logo_images->light_url) ==
                             "https://images-paymentMethod.meld.io/ACH/"
                             "logo_light.png";
                }),
            1);

  EXPECT_FALSE(ParsePaymentMethods(ParseJson(R"([
  {
    "name": "ACH",
    "paymentType": "BANK_TRANSFER",
    "logos": {
      "light": "https://images-paymentMethod.meld.io/ACH/logo_light.png"
    }
  }
  ])")));

  EXPECT_FALSE(ParsePaymentMethods(base::Value()));

  json = (R"({})");
  EXPECT_FALSE(ParsePaymentMethods(ParseJson(json)));
}

TEST(MeldIntegrationResponseParserUnitTest, Parse_FiatCurrencies) {
  std::string json(R"([
  {
    "currencyCode": "AFN",
    "name": "Afghani",
    "symbolImageUrl": "https://images-currency.meld.io/fiat/AFN/symbol.png"
  }])");

  auto fiat_currencies = ParseFiatCurrencies(ParseJson(json));
  EXPECT_TRUE(fiat_currencies);
  EXPECT_EQ(base::ranges::count_if(
                *fiat_currencies,
                [](const auto& item) {
                  return item->currency_code == "AFN" &&
                         item->name == "Afghani" &&
                         item->symbol_image_url ==
                             "https://images-currency.meld.io/fiat/"
                             "AFN/symbol.png";
                }),
            1);

  EXPECT_FALSE(ParsePaymentMethods(ParseJson(R"([
  {
    "name": "Afghani",
    "symbolImageUrl": "https://images-currency.meld.io/fiat/AFN/symbol.png"
  }])")));

  EXPECT_FALSE(ParseFiatCurrencies(base::Value()));

  json = (R"({})");
  EXPECT_FALSE(ParseFiatCurrencies(ParseJson(json)));
}

TEST(MeldIntegrationResponseParserUnitTest, Parse_CryptoCurrencies) {
  std::string json(R"([
  {
    "currencyCode": "USDT_KCC",
    "name": "#REF!",
    "chainCode": "KCC",
    "chainName": "KuCoin Community Chain",
    "chainId": "56",
    "contractAddress": "0xe41d2489571d322189246dafa5ebde1f4699f498",
    "symbolImageUrl": "https://images-currency.meld.io/crypto/USDT_KCC/symbol.png"
  }])");
  auto crypto_currencies = ParseCryptoCurrencies(ParseJson(json));
  EXPECT_TRUE(crypto_currencies);
  EXPECT_EQ(base::ranges::count_if(
                *crypto_currencies,
                [](const auto& item) {
                  return item->currency_code == "USDT_KCC" &&
                         item->name == "#REF!" && item->chain_code == "KCC" &&
                         item->chain_name == "KuCoin Community Chain" &&
                         item->chain_id == "0x38" &&
                         item->contract_address ==
                             "0xe41d2489571d322189246dafa5ebde1f4699f498" &&
                         item->symbol_image_url ==
                             "https://images-currency.meld.io/crypto/"
                             "USDT_KCC/symbol.png";
                }),
            1);

  EXPECT_FALSE(ParsePaymentMethods(ParseJson(R"([
  {
    "name": "#REF!",
    "chainCode": "KCC",
    "chainName": "KuCoin Community Chain",
    "chainId": "56",
    "contractAddress": "0xe41d2489571d322189246dafa5ebde1f4699f498",
    "symbolImageUrl": "https://images-currency.meld.io/crypto/USDT_KCC/symbol.png"
  }])")));

  EXPECT_FALSE(ParseCryptoCurrencies(base::Value()));

  json = (R"({})");
  EXPECT_FALSE(ParseCryptoCurrencies(ParseJson(json)));
}

TEST(MeldIntegrationResponseParserUnitTest, Parse_Countries) {
  std::string json(R"([
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
  }])");
  auto countries = ParseCountries(ParseJson(json));
  EXPECT_TRUE(countries);
  EXPECT_EQ(base::ranges::count_if(
                *countries,
                [](const auto& item) {
                  return item->country_code == "AF" &&
                         item->name == "Afghanistan" &&
                         item->flag_image_url ==
                             "https://images-country.meld.io/AF/flag.svg" &&
                         (*item->regions)[0]->region_code == "CA-AB" &&
                         (*item->regions)[0]->name == "Alberta" &&
                         (*item->regions)[1]->region_code == "CA-BC" &&
                         (*item->regions)[1]->name == "British Columbia";
                }),
            1);
  EXPECT_FALSE(ParseCountries(ParseJson(R"([
  {
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
