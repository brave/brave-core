/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/string_util.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/buy_and_sell_response_parser.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJson;

namespace brave_wallet {

TEST(BuyAndSellResponseParserUnitTest, Parse_ServiceProvider) {
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

  std::vector<mojom::ServiceProviderPtr> service_providers;
  EXPECT_TRUE(ParseServiceProviders(ParseJson(json), &service_providers));
  EXPECT_EQ(base::ranges::count_if(service_providers,
                                   [](const auto& item) {
                                     return item->name == "Banxa" &&
                                            item->service_provider == "BANXA" &&
                                            item->status == "LIVE" &&
                                            item->web_site_url ==
                                                "http://www.banxa.com" &&
                                            item->logo_images &&
                                            item->logo_images->dark_url == "https://images-serviceprovider.meld.io/BANXA/logo_dark.png" &&
                                            item->logo_images->dark_short_url == "https://images-serviceprovider.meld.io/BANXA/short_logo_dark.png" &&
                                            item->logo_images->light_url == "https://images-serviceprovider.meld.io/BANXA/logo_light.png" &&
                                            item->logo_images->light_short_url == "https://images-serviceprovider.meld.io/BANXA/short_logo_light.png"
                                            ;
                                   }),
            1);
  service_providers.clear();
  // Invalid json
  EXPECT_FALSE(ParseServiceProviders(base::Value(), &service_providers));

  // Valid json, missing required field
  json = (R"({})");
  EXPECT_FALSE(ParseServiceProviders(ParseJson(json), &service_providers));
}

TEST(BuyAndSellResponseParserUnitTest, Parse_MeldErrorResponse) {
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

  std::vector<std::string> errors;
  EXPECT_TRUE(ParseMeldErrorResponse(ParseJson(json), &errors));
  EXPECT_EQ(errors.size(), static_cast<size_t>(2));

  errors.clear();
  EXPECT_FALSE(ParseMeldErrorResponse(base::Value(), &errors));

  errors.clear();
  EXPECT_FALSE(ParseMeldErrorResponse(ParseJson((R"({})")), &errors));

  std::string json_only_msg(R"({
    "code": "BAD_REQUEST",
    "message": "Bad request",
    "errors": [],
    "requestId": "356dd2b40fa55037bfe9d190b6438f59",
    "timestamp": "2024-04-05T07:54:01.318455Z"
  })");
  errors.clear();
  EXPECT_TRUE(ParseMeldErrorResponse(ParseJson(json_only_msg), &errors));
  EXPECT_EQ(errors.size(), static_cast<size_t>(1));
}

TEST(BuyAndSellResponseParserUnitTest, Parse_CryptoQuotes) {
  std::string json(R"({
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
})");

  std::string error;
  std::vector<mojom::CryptoQuotePtr> quotes;
  EXPECT_TRUE(ParseCryptoQuotes(ParseJson(json), &quotes, &error));
  EXPECT_EQ(base::ranges::count_if(
                quotes,
                [](const auto& item) {
                  return item->transaction_type == "CRYPTO_PURCHASE" &&
                         item->exchange_rate == 75286 &&
                         item->source_amount == 50 &&
                         item->source_amount_without_fee == 43.97 &&
                         item->total_fee == 6.03 &&
                         item->payment_method == "APPLE_PAY" &&
                         item->destination_amount == 0.00066413 &&
                         item->service_provider_id == "TRANSAK";
                }),
            1);
  EXPECT_TRUE(error.empty());

  quotes.clear();
  EXPECT_FALSE(ParseCryptoQuotes(base::Value(), &quotes, &error));

  json = (R"({})");
  EXPECT_FALSE(ParseCryptoQuotes(ParseJson(json), &quotes, &error));
}

TEST(BuyAndSellResponseParserUnitTest, Parse_PaymentMethods) {
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

  std::vector<mojom::PaymentMethodPtr> payment_methods;
  EXPECT_TRUE(ParsePaymentMethods(ParseJson(json), &payment_methods));
  EXPECT_EQ(base::ranges::count_if(payment_methods,
                                   [](const auto& item) {
                                     return item->payment_method == "ACH" &&
                                            item->name == "ACH" &&
                                            item->payment_type ==
                                                "BANK_TRANSFER" &&
                                            item->logo_images &&
                                            item->logo_images->dark_short_url.empty() &&
                                            item->logo_images->light_short_url.empty() &&
                                            item->logo_images->dark_url == "https://images-paymentMethod.meld.io/ACH/logo_dark.png" &&
                                            item->logo_images->light_url == "https://images-paymentMethod.meld.io/ACH/logo_light.png";
                                   }),
            1);
  payment_methods.clear();
  EXPECT_FALSE(ParsePaymentMethods(base::Value(), &payment_methods));

  json = (R"({})");
  EXPECT_FALSE(ParsePaymentMethods(ParseJson(json), &payment_methods));
}

TEST(BuyAndSellResponseParserUnitTest, Parse_FiatCurrencies) {
  std::string json(R"([
  {
    "currencyCode": "AFN",
    "name": "Afghani",
    "symbolImageUrl": "https://images-currency.meld.io/fiat/AFN/symbol.png"
  }])");

  std::vector<mojom::FiatCurrencyPtr> fiat_currencies;
  EXPECT_TRUE(ParseFiatCurrencies(ParseJson(json), &fiat_currencies));
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
  fiat_currencies.clear();
  EXPECT_FALSE(ParseFiatCurrencies(base::Value(), &fiat_currencies));

  json = (R"({})");
  EXPECT_FALSE(ParseFiatCurrencies(ParseJson(json), &fiat_currencies));
}

TEST(BuyAndSellResponseParserUnitTest, Parse_CryptoCurrencies) {
  std::string json(R"([
  {
    "currencyCode": "USDT_KCC",
    "name": "#REF!",
    "chainCode": "KCC",
    "chainName": "KuCoin Community Chain",
    "chainId": "0",
    "contractAddress": "0xe41d2489571d322189246dafa5ebde1f4699f498",
    "symbolImageUrl": "https://images-currency.meld.io/crypto/USDT_KCC/symbol.png"
  }])");
  std::vector<mojom::CryptoCurrencyPtr> crypto_currencies;
  EXPECT_TRUE(ParseCryptoCurrencies(ParseJson(json), &crypto_currencies));
  EXPECT_EQ(base::ranges::count_if(
                crypto_currencies,
                [](const auto& item) {
                  return item->currency_code == "USDT_KCC" &&
                         item->name == "#REF!" && item->chain_code == "KCC" &&
                         item->chain_name == "KuCoin Community Chain" &&
                         item->chain_id == "0" &&
                         item->contract_address ==
                             "0xe41d2489571d322189246dafa5ebde1f4699f498" &&
                         item->symbol_image_url ==
                             "https://images-currency.meld.io/crypto/"
                             "USDT_KCC/symbol.png";
                }),
            1);
  crypto_currencies.clear();
  EXPECT_FALSE(ParseCryptoCurrencies(base::Value(), &crypto_currencies));

  json = (R"({})");
  EXPECT_FALSE(ParseCryptoCurrencies(ParseJson(json), &crypto_currencies));
}

TEST(BuyAndSellResponseParserUnitTest, Parse_Countries) {
  std::string json(R"([
  {
    "countryCode": "AF",
    "name": "Afghanistan",
    "flagImageUrl": "https://images-country.meld.io/AF/flag.svg",
    "regions": null
  }])");
  std::vector<mojom::CountryPtr> countries;
  EXPECT_TRUE(ParseCountries(ParseJson(json), &countries));
  EXPECT_EQ(base::ranges::count_if(
                countries,
                [](const auto& item) {
                  return item->country_code == "AF" &&
                         item->name == "Afghanistan" &&
                         item->flag_image_url ==
                             "https://images-country.meld.io/AF/flag.svg";
                }),
            1);
  countries.clear();
  EXPECT_FALSE(ParseCountries(base::Value(), &countries));

  json = (R"({})");
  EXPECT_FALSE(ParseCountries(ParseJson(json), &countries));
}

}  // namespace brave_wallet
