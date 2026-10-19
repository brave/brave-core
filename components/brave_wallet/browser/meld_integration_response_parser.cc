/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/meld_integration_response_parser.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/meld_integration_responses.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/meld_integration.mojom-forward.h"

namespace {

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

// Meld's chainId is only cross-checked for EVM chains, as Meld's ids of other
// chains don't follow Brave's format.
std::optional<std::pair<brave_wallet::mojom::CoinType, std::string_view>>
GetBraveChain(
    const brave_wallet::meld_integration_responses::CryptoCurrency& currency) {
  const auto it = brave_wallet::kMeldSupportedChains.find(currency.chain_code);
  if (it == brave_wallet::kMeldSupportedChains.end()) {
    return std::nullopt;
  }

  const auto& [coin, chain_id] = it->second;
  if (coin == brave_wallet::mojom::CoinType::DOT &&
      !brave_wallet::IsPolkadotEnabled()) {
    return std::nullopt;
  }

  if (coin == brave_wallet::mojom::CoinType::ETH && currency.chain_id) {
    uint64_t meld_chain_id = 0;
    if (!base::StringToUint64(*currency.chain_id, &meld_chain_id) ||
        brave_wallet::Uint256ValueToHex(meld_chain_id) != chain_id) {
      return std::nullopt;
    }
  }

  return it->second;
}

// Meld may suffix currency codes with their chain code, e.g. "USDT_POLYGON".
std::string GetCurrencySymbol(std::string_view currency_code,
                              std::string_view chain_code) {
  if (const auto symbol =
          base::RemoveSuffix(currency_code, base::StrCat({"_", chain_code}))) {
    return std::string(*symbol);
  }
  return std::string(currency_code);
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

    const auto& logos = service_provider_value->logos;
    auto sp = mojom::MeldServiceProvider::New(
        service_provider_value->name, service_provider_value->service_provider,
        service_provider_value->status, service_provider_value->website_url,
        service_provider_value->categories,
        ParseOptionalMapOfStrings(service_provider_value->category_statuses),
        logos ? logos->dark_short : std::nullopt,
        logos ? logos->light_short : std::nullopt);

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
  //     "providerMethod": "card",
  //     "method": "CREDIT_DEBIT_CARD",
  //     "name": "Credit / Debit Card",
  //     "type": "CARD",
  //     "logo": {
  //       "dark": "https://images-paymentMethod.meld.io/CARD/logo_dark.png",
  //       "light": "https://images-paymentMethod.meld.io/CARD/logo_light.png"
  //     },
  //     "headlessSupported": false
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

    const auto& logo = payment_method_value->logo;
    auto pm = mojom::MeldPaymentMethod::New(
        payment_method_value->method, payment_method_value->name,
        payment_method_value->type, logo ? logo->dark : std::nullopt,
        logo ? logo->light : std::nullopt);

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

  const auto response =
      meld_integration_responses::FiatCurrenciesResponse::FromValue(json_value);
  if (!response) {
    return std::nullopt;
  }

  std::vector<mojom::MeldFiatCurrencyPtr> fiat_currencies;
  for (const auto& fiat_currency_value : response->currencies) {
    auto fc = mojom::MeldFiatCurrency::New(fiat_currency_value.currency_code,
                                           fiat_currency_value.name,
                                           fiat_currency_value.symbol);

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
  //       "currencyCode": "USDT_KCC",
  //       "name": "#REF!",
  //       "decimalPlaces": 6,
  //       "type": "CRYPTO",
  //       "chainCode": "KCC",
  //       "chainId": null,
  //       "contract": null,
  //       "symbol":
  //       "https://images-currency.meld.io/crypto/USDT_KCC/symbol.png"
  //     },
  //     {
  //       "currencyCode": "00",
  //       "name": "00 Token",
  //       "decimalPlaces": 18,
  //       "type": "CRYPTO",
  //       "chainCode": "ETH",
  //       "chainId": "1",
  //       "contract": null,
  //       "symbol": "https://images-currency.meld.io/crypto/00/symbol.png"
  //     }
  //   ]
  // }
  const auto response =
      meld_integration_responses::CryptoCurrenciesResponse::FromValue(
          json_value);
  if (!response) {
    return std::nullopt;
  }

  std::vector<mojom::MeldCryptoCurrencyPtr> crypto_currencies;
  for (const auto& crypto_currency_value : response->currencies) {
    const auto brave_chain = GetBraveChain(crypto_currency_value);
    if (!brave_chain) {
      continue;
    }

    const auto& chain_code = crypto_currency_value.chain_code;
    auto cc = mojom::MeldCryptoCurrency::New(
        brave_chain->first, std::string(brave_chain->second),
        GetCurrencySymbol(crypto_currency_value.currency_code, chain_code),
        crypto_currency_value.currency_code, crypto_currency_value.name,
        chain_code, crypto_currency_value.contract,
        crypto_currency_value.symbol);

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
  //       "flag": "<emoji>",
  //       "subdivisions": []
  //     },
  //     {
  //       "countryCode": "AL",
  //       "name": "Albania",
  //       "flag": "<emoji>",
  //       "subdivisions": []
  //     }
  //   ]
  // }
  const auto response =
      meld_integration_responses::CountriesResponse::FromValue(json_value);
  if (!response) {
    return std::nullopt;
  }

  std::vector<mojom::MeldCountryPtr> countries;
  for (const auto& country_value : response->countries) {
    auto country =
        mojom::MeldCountry::New(country_value.country_code, country_value.name);

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
