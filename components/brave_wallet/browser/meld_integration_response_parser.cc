/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/meld_integration_response_parser.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/meld_integration_responses.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/meld_integration.mojom-forward.h"

namespace {

brave_wallet::mojom::MeldLogoImagesPtr ParseMeldLogos(
    const std::optional<brave_wallet::meld_integration_responses::Logos>&
        logos) {
  if (!logos) {
    return nullptr;
  }

  return brave_wallet::mojom::MeldLogoImages::New(
      logos->dark, logos->dark_short, logos->light, logos->light_short);
}

base::flat_map<std::string, std::string> ParseOptionalMapOfStrings(
    const base::Value& val) {
  base::flat_map<std::string, std::string> result;
  if (!val.is_dict()) {
    return result;
  }

  for (const auto [key, value] : val.GetDict()) {
    result.insert_or_assign(key, value.GetString());
  }

  return result;
}

}  //  namespace

namespace brave_wallet {

std::optional<std::vector<mojom::MeldServiceProviderPtr>> ParseServiceProviders(
    const base::Value& json_value) {
  // Parses results like this:
  // {
  //    "categories": [ "CRYPTO_ONRAMP" ],
  //    "categoryStatuses": {
  //       "CRYPTO_ONRAMP": "LIVE"
  //    },
  //    "logos": {
  //       "dark": "https://images-serviceprovider.meld.io/BANXA/logo_dark.png",
  //       "darkShort":
  //       "https://images-serviceprovider.meld.io/BANXA/short_logo_dark.png",
  //       "light":
  //       "https://images-serviceprovider.meld.io/BANXA/logo_light.png",
  //       "lightShort":
  //       "https://images-serviceprovider.meld.io/BANXA/short_logo_light.png"
  //    },
  //    "name": "Banxa",
  //    "serviceProvider": "BANXA",
  //    "status": "LIVE",
  //    "websiteUrl": "http://www.banxa.com"
  // }

  if (!json_value.is_list()) {
    return std::nullopt;
  }

  std::vector<mojom::MeldServiceProviderPtr> service_providers;
  for (const auto& sp_item : json_value.GetList()) {
    const auto service_provider_value =
        meld_integration_responses::ServiceProvider::FromValue(sp_item);
    if (!service_provider_value) {
      return std::nullopt;
    }

    auto logos = ParseMeldLogos(service_provider_value->logos);
    auto sp = mojom::MeldServiceProvider::New(
        service_provider_value->name, service_provider_value->service_provider,
        service_provider_value->status, service_provider_value->website_url,
        service_provider_value->categories,
        ParseOptionalMapOfStrings(service_provider_value->category_statuses),
        std::move(logos));

    service_providers.emplace_back(std::move(sp));
  }

  if (service_providers.empty()) {
    return std::nullopt;
  }

  return service_providers;
}

std::optional<std::vector<std::string>> ParseMeldErrorResponse(
    const base::Value& json_value) {
  // Parses results like this:
  // {
  //     "code": "BAD_REQUEST",
  //     "message": "Bad request",
  //     "errors": [
  //         "[amount] Must be a decimal value greater than zero"
  //     ],
  //     "requestId": "eb6aaa76bd7103cf6c5b090610c31913",
  //     "timestamp": "2022-01-19T20:32:30.784928Z"
  // }

  const auto meld_error_value =
      meld_integration_responses::MeldError::FromValue(json_value);
  if (!meld_error_value) {
    return std::nullopt;
  }

  std::vector<std::string> errors;
  if (meld_error_value->errors && !meld_error_value->errors->empty()) {
    errors.assign(meld_error_value->errors->begin(),
                  meld_error_value->errors->end());
  }

  if (meld_error_value->message.has_value() && errors.empty()) {
    errors.emplace_back(*meld_error_value->message);
  }

  if (errors.empty()) {
    return std::nullopt;
  }

  return errors;
}

base::expected<std::vector<mojom::MeldCryptoQuotePtr>, std::string>
ParseCryptoQuotes(const base::Value& json_value) {
  // Parses results like this:
  // {
  //   "quotes": [
  //     {
  //       "transactionType": "CRYPTO_PURCHASE",
  //       "sourceAmount": 50,
  //       "sourceAmountWithoutFees": 43.97,
  //       "fiatAmountWithoutFees": 43.97,
  //       "destinationAmountWithoutFees": null,
  //       "sourceCurrencyCode": "USD",
  //       "countryCode": "US",
  //       "totalFee": 6.03,
  //       "networkFee": 3.53,
  //       "transactionFee": 2,
  //       "destinationAmount": 0.00066413,
  //       "destinationCurrencyCode": "BTC",
  //       "exchangeRate": 75286,
  //       "paymentMethodType": "APPLE_PAY",
  //       "customerScore": 20,
  //       "serviceProvider": "TRANSAK"
  //     }
  //   ],
  //   "message": null,
  //   "error": null
  // }
  const auto quote_resp_value =
      meld_integration_responses::CryptoQuoteResponse::FromValue(json_value);
  if (!quote_resp_value) {
    return base::unexpected("");
  }

  if (quote_resp_value->error) {
    return base::unexpected(*quote_resp_value->error);
  }

  std::vector<mojom::MeldCryptoQuotePtr> quotes;
  if (!quote_resp_value->quotes) {
    return base::ok(std::move(quotes));
  }

  for (const auto& quote_value : *quote_resp_value->quotes) {
    auto quote = mojom::MeldCryptoQuote::New(
        quote_value.transaction_type, quote_value.exchange_rate,
        quote_value.transaction_fee, quote_value.source_currency_code,
        quote_value.source_amount, quote_value.source_amount_without_fees,
        quote_value.fiat_amount_without_fees, quote_value.total_fee,
        quote_value.network_fee, quote_value.payment_method_type,
        quote_value.destination_currency_code, quote_value.destination_amount,
        quote_value.destination_amount_without_fees, quote_value.customer_score,
        quote_value.service_provider, quote_value.country_code);

    quotes.emplace_back(std::move(quote));
  }
  return base::ok(std::move(quotes));
}

std::optional<std::vector<mojom::MeldPaymentMethodPtr>> ParsePaymentMethods(
    const base::Value& json_value) {
  // Parses results like this:
  // [
  //   {
  //     "countryCode": "US",
  //     "providerMethod": "ach",
  //     "method": "ACH",
  //     "name": "ACH Bank Transfer",
  //     "type": "BANK_TRANSFER",
  //     "headlessSupported": true,
  //     "logo": {
  //       "dark": "https://images-paymentMethod.meld.io/ACH/logo_dark.png",
  //       "light": "https://images-paymentMethod.meld.io/ACH/logo_light.png"
  //     }
  //   }
  // ]
  if (!json_value.is_list()) {
    return std::nullopt;
  }
  std::vector<mojom::MeldPaymentMethodPtr> payment_methods;
  for (const auto& pm_item : json_value.GetList()) {
    const auto payment_method_value =
        meld_integration_responses::PaymentMethod::FromValue(pm_item);
    if (!payment_method_value) {
      return std::nullopt;
    }

    auto logo = ParseMeldLogos(payment_method_value->logo);
    auto pm = mojom::MeldPaymentMethod::New(
        payment_method_value->method, payment_method_value->name,
        payment_method_value->payment_type, std::move(logo));

    payment_methods.emplace_back(std::move(pm));
  }

  if (payment_methods.empty()) {
    return std::nullopt;
  }

  return payment_methods;
}

std::optional<std::vector<mojom::MeldFiatCurrencyPtr>> ParseFiatCurrencies(
    const base::Value& json_value) {
  // Parses results like this:
  // {
  //   "currencies": [
  //     {
  //       "currencyCode": "AFN",
  //       "name": "Afghani",
  //       "decimalPlaces": 2,
  //       "type": "FIAT",
  //       "symbol": "https://images-currency.meld.io/fiat/AFN/symbol.png"
  //     }
  //   ]
  // }

  const auto response_value =
      meld_integration_responses::FiatCurrenciesResponse::FromValue(
          json_value);
  if (!response_value || !response_value->currencies) {
    return std::nullopt;
  }

  std::vector<mojom::MeldFiatCurrencyPtr> fiat_currencies;
  for (const auto& fiat_currency_value : *response_value->currencies) {
    auto fc = mojom::MeldFiatCurrency::New(
        fiat_currency_value.currency_code, fiat_currency_value.name,
        fiat_currency_value.decimal_places, fiat_currency_value.symbol);

    fiat_currencies.emplace_back(std::move(fc));
  }

  return fiat_currencies;
}

std::optional<std::vector<mojom::MeldCryptoCurrencyPtr>> ParseCryptoCurrencies(
    const base::Value& json_value) {
  // Parses results like this:
  // {
  //   "currencies": [
  //     {
  //       "currencyCode": "USDC",
  //       "name": "USD Coin",
  //       "decimalPlaces": 6,
  //       "type": "CRYPTO",
  //       "symbol": "https://images-currency.meld.io/crypto/USDC/symbol.png",
  //       "chainCode": "ETH",
  //       "contract": "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48"
  //     }
  //   ]
  // }
  const auto response_value =
      meld_integration_responses::CryptoCurrenciesResponse::FromValue(
          json_value);
  if (!response_value || !response_value->currencies) {
    return std::nullopt;
  }

  std::vector<mojom::MeldCryptoCurrencyPtr> crypto_currencies;
  for (const auto& crypto_currency_value : *response_value->currencies) {
    std::optional<std::string> chain_id_hex;
    if (int chain_id_as_num;
        crypto_currency_value.chain_id &&
        base::StringToInt(*crypto_currency_value.chain_id, &chain_id_as_num)) {
      chain_id_hex = Uint256ValueToHex(chain_id_as_num);
    }

    auto cc = mojom::MeldCryptoCurrency::New(
        crypto_currency_value.currency_code, crypto_currency_value.name,
        crypto_currency_value.decimal_places, crypto_currency_value.chain_code,
        crypto_currency_value.chain_name, chain_id_hex,
        crypto_currency_value.contract, crypto_currency_value.symbol);

    crypto_currencies.emplace_back(std::move(cc));
  }

  return crypto_currencies;
}

std::optional<std::vector<mojom::MeldCountryPtr>> ParseCountries(
    const base::Value& json_value) {
  // Parses results like this:
  // {
  //   "countries": [
  //     {
  //       "countryCode": "AF",
  //       "name": "Afghanistan",
  //       "flag": "https://images-country.meld.io/AF/flag.svg"
  //     },
  //     {
  //       "countryCode": "AL",
  //       "name": "Albania",
  //       "flag": "https://images-country.meld.io/AL/flag.svg"
  //     }
  //   ]
  // }
  const auto response_value =
      meld_integration_responses::CountriesResponse::FromValue(json_value);
  if (!response_value || !response_value->countries) {
    return std::nullopt;
  }

  std::vector<mojom::MeldCountryPtr> countries;
  for (const auto& country_value : *response_value->countries) {
    auto country = mojom::MeldCountry::New(
        country_value.country_code, country_value.name, country_value.flag);

    countries.emplace_back(std::move(country));
  }

  return countries;
}

mojom::MeldCryptoWidgetPtr ParseCryptoWidgetCreate(
    const base::Value& json_value) {
  // Parses results like this:
  // {
  //   "id": "WXDmJRFbxfUYgRi3Skbqd3",
  //   "externalSessionId": null,
  //   "externalCustomerId": null,
  //   "customerId": "WXDmJQhKFEeFt5jSeAz7gh",
  //   "widgetUrl":
  //   "https://sb.meldcrypto.com?token=token_value",
  //   "token": "token_value"
  // }

  if (!json_value.is_dict()) {
    return nullptr;
  }

  const auto cw_value =
      meld_integration_responses::CryptoWidgetResult::FromValue(json_value);
  if (!cw_value) {
    return nullptr;
  }

  if (cw_value->id.empty() || cw_value->customer_id.empty() ||
      cw_value->widget_url.empty() || cw_value->token.empty()) {
    return nullptr;
  }

  if (const auto widget_url = GURL(cw_value->widget_url);
      !widget_url.is_valid() || !widget_url.SchemeIsHTTPOrHTTPS()) {
    return nullptr;
  }

  return mojom::MeldCryptoWidget::New(
      cw_value->id, cw_value->external_session_id,
      cw_value->external_customer_id, cw_value->customer_id,
      cw_value->widget_url, cw_value->token);
}

}  // namespace brave_wallet
