/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/meld_integration_response_parser.h"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "base/ranges/algorithm.h"
#include "brave/components/brave_wallet/browser/meld_integration_responses.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "tools/json_schema_compiler/util.h"

namespace {

std::optional<brave_wallet::mojom::LogoImagesPtr> ParseMeldLogos(const std::optional<base::Value>& logos) {
  if (!logos || !logos->is_dict()) {
    return std::nullopt;
  }

  const auto logos_value =
      brave_wallet::meld_integration_responses::Logos::FromValue(*logos);
  if (!logos_value) {
    return std::nullopt;
  }

  auto logo_images = brave_wallet::mojom::LogoImages::New();
  if (logos_value->dark && logos_value->dark->is_string()) {
    logo_images->dark_url = logos_value->dark->GetString();
  }
  if (logos_value->dark_short && logos_value->dark_short->is_string()) {
    logo_images->dark_short_url = logos_value->dark_short->GetString();
  }
  if (logos_value->light && logos_value->light->is_string()) {
    logo_images->light_url = logos_value->light->GetString();
  }
  if (logos_value->light_short && logos_value->light_short->is_string()) {
    logo_images->light_short_url = logos_value->light_short->GetString();
  }

  return logo_images;
}

std::optional<std::string> ParseOptionalString(
    const std::optional<base::Value>& value) {
  if (!value || !value->is_string()) {
    return std::nullopt;
  }

  return value->GetString();
}

std::optional<double> ParseOptionDouble(const std::optional<base::Value>& value) {
  if (!value) {
    return std::nullopt;
  }

  if(value->is_double()) {
    return value->GetDouble();
  }

  if(value->is_int()) {
    return static_cast<double>(value->GetInt());
  }

  return std::nullopt;
}

void ParseMeldRegions(
    const std::optional<base::Value>& idl_regions,
    brave_wallet::mojom::Country* country) {
  if (!idl_regions || !idl_regions->is_list() || !country) {
    return;
  }

  if(!country->regions) {
    country->regions = std::vector<brave_wallet::mojom::RegionPtr>();
  }

  for(const auto& item : idl_regions->GetList()) {
    const auto region_value =
        brave_wallet::meld_integration_responses::Region::FromValue(item);
    if (!region_value) {
      return;
    }

    auto reg = brave_wallet::mojom::Region::New();
    reg->region_code = ParseOptionalString(region_value->region_code);
    reg->name = ParseOptionalString(region_value->name);
    country->regions->emplace_back(std::move(reg));
  }
}

std::optional<std::vector<std::string>> ParseOptionalVectorOfStrings(
    const std::optional<base::Value>& val) {
  if (!val || !val->is_list()) {
    return std::nullopt;
  }

  std::vector<std::string> result;
  for (const auto& item : val->GetList()) {
    if (!item.is_string()) {
      continue;
    }
    result.emplace_back(item.GetString());
  }

  if (result.empty()) {
    return std::nullopt;
  }

  return result;
}

std::optional<base::flat_map<std::string, std::string>> ParseOptionalMapOfStrings(
    const std::optional<base::Value>& val) {
  if (!val || !val->is_dict()) {
    return std::nullopt;
  }

  base::flat_map<std::string, std::string> result;
  for (const auto [key, value] : val->GetDict()) {
    result.insert_or_assign(key, value.GetString());
  }
  if (result.empty()) {
    return std::nullopt;
  }

  return result;
}

}  //  namespace

namespace brave_wallet {

std::optional<std::vector<mojom::ServiceProviderPtr>> ParseServiceProviders(
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
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is not a list";
    return std::nullopt;
  }

  std::vector<mojom::ServiceProviderPtr> service_providers;
  for (const auto& sp_item : json_value.GetList()) {
    auto sp = mojom::ServiceProvider::New();
    const auto service_provider_value =
        meld_integration_responses::ServiceProvider::FromValue(sp_item);
    if (!service_provider_value) {
      LOG(ERROR)
          << "Invalid response, could not parse JSON, JSON is not a dict";
      return std::nullopt;
    }

    sp->name = ParseOptionalString(service_provider_value->name);
    sp->service_provider = ParseOptionalString(service_provider_value->service_provider);
    sp->status = ParseOptionalString(service_provider_value->status);
    sp->categories = ParseOptionalVectorOfStrings(service_provider_value->categories);
    sp->web_site_url = ParseOptionalString(service_provider_value->website_url);
    sp->category_statuses = ParseOptionalMapOfStrings(service_provider_value->category_statuses);

    if(auto logo_images = ParseMeldLogos(service_provider_value->logos); logo_images) {
      sp->logo_images = std::move(*logo_images);
    }

    service_providers.emplace_back(std::move(sp));
  }

  if(service_providers.empty()) {
    return std::nullopt;
  }

  return service_providers;
}

std::optional<std::vector<std::string>> ParseMeldErrorResponse(const base::Value& json_value) {
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
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is not a dict";
    return std::nullopt;
  }

  std::vector<std::string> errors;
  if (meld_error_value->errors &&
      !meld_error_value->errors->empty()) {
    errors.assign(meld_error_value->errors->begin(),
                   meld_error_value->errors->end());
  }

  if (meld_error_value->message.has_value() && errors.empty()) {
    errors.emplace_back(*meld_error_value->message);
  }

  if(errors.empty()){
    return std::nullopt;
  }

  return errors;
}

std::optional<std::vector<mojom::CryptoQuotePtr>> ParseCryptoQuotes(
    const base::Value& json_value,
    std::string* error) {
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
  DCHECK(error);
  const auto quote_resp_value =
      meld_integration_responses::CryptoQuoteResponse::FromValue(json_value);
  if (!quote_resp_value) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is not a dict";
    return std::nullopt;
  }

  if (quote_resp_value->error && quote_resp_value->error->is_string()) {
    *error = quote_resp_value->error->GetString();
  }

  if (!quote_resp_value->quotes || !quote_resp_value->quotes->is_list()) {
    return std::nullopt;
  }

  std::vector<mojom::CryptoQuotePtr> quotes;
  for (const auto& item : quote_resp_value->quotes->GetList()) {
    const auto quote_value =
        meld_integration_responses::CryptoQuote::FromValue(item);
    if (!quote_value) {
      LOG(ERROR)
          << "Invalid response, could not parse JSON, JSON is not a dict";
      return std::nullopt;
    }

    auto quote = mojom::CryptoQuote::New();

    quote->transaction_type =
        ParseOptionalString(quote_value->transaction_type);
    quote->source_amount = ParseOptionDouble(quote_value->source_amount);
    quote->source_amount_without_fee =
        ParseOptionDouble(quote_value->source_amount_without_fees);
    quote->fiat_amount_without_fees =
        ParseOptionDouble(quote_value->fiat_amount_without_fees);
    quote->destination_amount_without_fees =
        ParseOptionDouble(quote_value->destination_amount_without_fees);
    quote->source_currency_code =
        ParseOptionalString(quote_value->source_currency_code);
    quote->country_code = ParseOptionalString(quote_value->country_code);
    quote->total_fee = ParseOptionDouble(quote_value->total_fee);
    quote->network_fee = ParseOptionDouble(quote_value->network_fee);
    quote->transaction_fee = ParseOptionDouble(quote_value->transaction_fee);
    quote->destination_amount =
        ParseOptionDouble(quote_value->destination_amount);
    quote->destination_currency_code =
        ParseOptionalString(quote_value->destination_currency_code);
    quote->exchange_rate = ParseOptionDouble(quote_value->exchange_rate);
    quote->payment_method =
        ParseOptionalString(quote_value->payment_method_type);
    quote->customer_score = ParseOptionDouble(quote_value->customer_score);
    quote->service_provider =
        ParseOptionalString(quote_value->service_provider);
    quotes.emplace_back(std::move(quote));
  }

  if(quotes.empty()) {
    return std::nullopt;
  }

  return quotes;
}

std::optional<std::vector<mojom::PaymentMethodPtr>> ParsePaymentMethods(
    const base::Value& json_value) {
  // Parses results like this:
  // [
  //   {
  //     "paymentMethod": "ACH",
  //     "name": "ACH",
  //     "paymentType": "BANK_TRANSFER",
  //     "logos": {
  //       "dark": "https://images-paymentMethod.meld.io/ACH/logo_dark.png",
  //       "light": "https://images-paymentMethod.meld.io/ACH/logo_light.png"
  //     }
  //   }
  // ]
  if (!json_value.is_list()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is not a list";
    return std::nullopt;
  }
  std::vector<mojom::PaymentMethodPtr> payment_methods;
  for (const auto& pm_item : json_value.GetList()) {
    const auto payment_method_value =
        meld_integration_responses::PaymentMethod::FromValue(pm_item);
    if (!payment_method_value) {
      LOG(ERROR) << "Invalid response, could not parse JSON, JSON is not a dict";
      return std::nullopt;
    }

    auto pm = mojom::PaymentMethod::New();
    pm->name = ParseOptionalString(payment_method_value->name);
    pm->payment_method = ParseOptionalString(payment_method_value->payment_method);
    pm->payment_type = ParseOptionalString(payment_method_value->payment_type);

    if(auto logo_images = ParseMeldLogos(payment_method_value->logos); logo_images) {
      pm->logo_images = std::move(*logo_images);
    }
    
    payment_methods.emplace_back(std::move(pm));
  }

  if(payment_methods.empty()) {
    return std::nullopt;
  }

  return payment_methods;
}

std::optional<std::vector<mojom::FiatCurrencyPtr>> ParseFiatCurrencies(
    const base::Value& json_value) {
  // Parses results like this:
  // [
  //   {
  //     "currencyCode": "AFN",
  //     "name": "Afghani",
  //     "symbolImageUrl": "https://images-currency.meld.io/fiat/AFN/symbol.png"
  //   }
  // ]

  if (!json_value.is_list()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is not a list";
    return std::nullopt;
  }
  std::vector<mojom::FiatCurrencyPtr> fiat_currencies;
  for (const auto& fc_item : json_value.GetList()) {
    const auto fiat_currency_value =
        meld_integration_responses::FiatCurrency::FromValue(fc_item);
    if (!fiat_currency_value) {
      LOG(ERROR) << "Invalid response, could not parse JSON";
      return std::nullopt;
    }

    auto fc = mojom::FiatCurrency::New();
    fc->name = ParseOptionalString(fiat_currency_value->name);
    fc->currency_code = ParseOptionalString(fiat_currency_value->currency_code);
    fc->symbol_image_url = ParseOptionalString(fiat_currency_value->symbol_image_url);

    fiat_currencies.emplace_back(std::move(fc));
  }

  return fiat_currencies;
}

std::optional<std::vector<mojom::CryptoCurrencyPtr>> ParseCryptoCurrencies(
    const base::Value& json_value) {
  // Parses results like this:
  // [
  //   {
  //     "currencyCode": "USDT_KCC",
  //     "name": "#REF!",
  //     "chainCode": "KCC",
  //     "chainName": "KuCoin Community Chain",
  //     "chainId": null,
  //     "contractAddress": null,
  //     "symbolImageUrl":
  //     "https://images-currency.meld.io/crypto/USDT_KCC/symbol.png"
  //   },
  //   {
  //     "currencyCode": "00",
  //     "name": "00 Token",
  //     "chainCode": "ETH",
  //     "chainName": "Ethereum",
  //     "chainId": "1",
  //     "contractAddress": null,
  //     "symbolImageUrl":
  //     "https://images-currency.meld.io/crypto/00/symbol.png"
  //   }
  // ]
  if (!json_value.is_list()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is not a list";
    return std::nullopt;
  }
  std::vector<mojom::CryptoCurrencyPtr> crypto_currencies;
  for (const auto& cc_item : json_value.GetList()) {
    const auto crypto_currency_value =
        meld_integration_responses::CryptoCurrency::FromValue(cc_item);
    if (!crypto_currency_value) {
      LOG(ERROR) << "Invalid response, could not parse JSON";
      return std::nullopt;
    }

    auto cc = mojom::CryptoCurrency::New();
    cc->name = ParseOptionalString(crypto_currency_value->name);
    cc->currency_code = ParseOptionalString(crypto_currency_value->currency_code);
    cc->chain_code = ParseOptionalString(crypto_currency_value->chain_code);
    cc->chain_name = ParseOptionalString(crypto_currency_value->chain_name);
    cc->chain_id = ParseOptionalString(crypto_currency_value->chain_id);
    cc->contract_address = ParseOptionalString(crypto_currency_value->contract_address);
    cc->symbol_image_url = ParseOptionalString(crypto_currency_value->symbol_image_url);

    crypto_currencies.emplace_back(std::move(cc));
  }

  return crypto_currencies;
}

std::optional<std::vector<mojom::CountryPtr>> ParseCountries(
    const base::Value& json_value) {
  // Parses results like this:
  // [
  //   {
  //     "countryCode": "AF",
  //     "name": "Afghanistan",
  //     "flagImageUrl": "https://images-country.meld.io/AF/flag.svg",
  //     "regions": null
  //   },
  //   {
  //     "countryCode": "AL",
  //     "name": "Albania",
  //     "flagImageUrl": "https://images-country.meld.io/AL/flag.svg",
  //     "regions": null
  //   }
  // ]
  if (!json_value.is_list()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is not a list";
    return std::nullopt;
  }
  std::vector<mojom::CountryPtr> countries;
  for (const auto& country_item : json_value.GetList()) {
    const auto country_value =
        meld_integration_responses::Country::FromValue(country_item);
    if (!country_value) {
      LOG(ERROR) << "Invalid response, could not parse JSON";
      return std::nullopt;
    }

    auto country = mojom::Country::New();
    country->name = ParseOptionalString(country_value->name);
    country->country_code = ParseOptionalString(country_value->country_code);
    country->flag_image_url = ParseOptionalString(country_value->flag_image_url);
    ParseMeldRegions(country_value->regions, country.get());

   countries.emplace_back(std::move(country));
  }

  return countries;
}

}  // namespace brave_wallet
