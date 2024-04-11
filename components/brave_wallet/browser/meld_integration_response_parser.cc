/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/meld_integration_response_parser.h"

#include <string>
#include <utility>
#include <vector>
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"

namespace {

bool ParseMeldLogos(const base::Value::Dict* logos,
                    brave_wallet::mojom::LogoImages* logo_images) {
  if (!logos) {
    return false;
  }
  if (const auto* dark_logo = logos->FindString("dark")) {
    logo_images->dark_url = *dark_logo;
  }
  if (const auto* dark_short_logo = logos->FindString("darkShort")) {
    logo_images->dark_short_url = *dark_short_logo;
  }
  if (const auto* light_logo = logos->FindString("light")) {
    logo_images->light_url = *light_logo;
  }
  if (const auto* light_short_logo = logos->FindString("lightShort")) {
    logo_images->light_short_url = *light_short_logo;
  }

  return true;
}

}  //  namespace

namespace brave_wallet {
bool ParseServiceProviders(
    const base::Value& json_value,
    std::vector<mojom::ServiceProviderPtr>* service_providers) {
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

  DCHECK(service_providers);

  if (!json_value.is_list()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is not a list";
    return false;
  }

  for (const auto& sp_item : json_value.GetList()) {
    if (!sp_item.is_dict()) {
      LOG(ERROR)
          << "Invalid response, could not parse JSON, JSON is not a dict";
      return false;
    }

    auto sp = mojom::ServiceProvider::New();
    const std::string* sp_name = sp_item.GetDict().FindString("name");
    if (!sp_name) {
      return false;
    }
    sp->name = *sp_name;

    const std::string* sp_status = sp_item.GetDict().FindString("status");
    if (!sp_status) {
      return false;
    }
    sp->status = *sp_status;

    const std::string* sp_service_provider =
        sp_item.GetDict().FindString("serviceProvider");
    if (!sp_service_provider) {
      return false;
    }
    sp->service_provider = *sp_service_provider;

    const std::string* sp_site_url = sp_item.GetDict().FindString("websiteUrl");
    if (!sp_site_url) {
      return false;
    }
    sp->web_site_url = *sp_site_url;

    sp->logo_images = mojom::LogoImages::New();
    if (const auto* logos = sp_item.GetDict().FindDict("logos");
        !ParseMeldLogos(logos, sp->logo_images.get())) {
      return false;
    }

    service_providers->emplace_back(std::move(sp));
  }

  return true;
}

bool ParseMeldErrorResponse(const base::Value& json_value,
                            std::vector<std::string>* errors) {
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
  DCHECK(errors);
  if (!json_value.is_dict()) {
    return false;
  }

  const auto& response_error_dict = json_value.GetDict();

  if (const auto* response_errors = response_error_dict.FindList("errors");
      response_errors) {
    for (const auto& err : *response_errors) {
      errors->emplace_back(err.GetString());
    }
  }

  if (const auto* response_error_message =
          response_error_dict.FindString("message");
      response_error_message && errors->empty()) {
    errors->emplace_back(*response_error_message);
  }

  return !errors->empty();
}

bool ParseCryptoQuotes(const base::Value& json_value,
                       std::vector<mojom::CryptoQuotePtr>* quotes,
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
  DCHECK(quotes);
  DCHECK(error);

  if (!json_value.is_dict()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is not a dict";
    return false;
  }

  const auto& response_dict = json_value.GetDict();
  if (const auto* response_error = response_dict.FindString("error")) {
    *error = *response_error;
  }

  const auto* response_quotes = response_dict.FindList("quotes");
  if (!response_quotes) {
    return false;
  }

  for (const auto& item : *response_quotes) {
    if (!item.is_dict()) {
      LOG(ERROR)
          << "Invalid response, could not parse JSON, JSON is not a dict";
      return false;
    }

    auto quote = mojom::CryptoQuote::New();
    const std::string* quote_tt = item.GetDict().FindString("transactionType");
    if (!quote_tt) {
      return false;
    }
    quote->transaction_type = *quote_tt;

    const auto quote_er = item.GetDict().FindDouble("exchangeRate");
    if (!quote_er) {
      return false;
    }
    quote->exchange_rate = *quote_er;

    const auto quote_amount = item.GetDict().FindDouble("sourceAmount");
    if (!quote_amount) {
      return false;
    }
    quote->source_amount = *quote_amount;

    const auto quote_amount_without_fee =
        item.GetDict().FindDouble("sourceAmountWithoutFees");
    if (!quote_amount_without_fee) {
      return false;
    }
    quote->source_amount_without_fee = *quote_amount_without_fee;

    const auto quote_total_fee = item.GetDict().FindDouble("totalFee");
    if (!quote_total_fee) {
      return false;
    }
    quote->total_fee = *quote_total_fee;

    const std::string* quote_pp =
        item.GetDict().FindString("paymentMethodType");
    if (!quote_pp) {
      return false;
    }
    quote->payment_method = *quote_pp;

    const auto quote_dest_amount =
        item.GetDict().FindDouble("destinationAmount");
    if (!quote_dest_amount) {
      return false;
    }
    quote->destination_amount = *quote_dest_amount;

    const std::string* quote_sp = item.GetDict().FindString("serviceProvider");
    if (!quote_sp) {
      return false;
    }
    quote->service_provider_id = *quote_sp;

    quotes->emplace_back(std::move(quote));
  }

  return true;
}

bool ParsePaymentMethods(
    const base::Value& json_value,
    std::vector<mojom::PaymentMethodPtr>* payment_methods) {
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
  DCHECK(payment_methods);

  if (!json_value.is_list()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is not a list";
    return false;
  }

  for (const auto& pm_item : json_value.GetList()) {
    if (!pm_item.is_dict()) {
      LOG(ERROR)
          << "Invalid response, could not parse JSON, JSON is not a dict";
      return false;
    }

    auto pm = mojom::PaymentMethod::New();
    const std::string* pm_name = pm_item.GetDict().FindString("name");
    if (!pm_name) {
      return false;
    }
    pm->name = *pm_name;

    const std::string* pm_payment_method =
        pm_item.GetDict().FindString("paymentMethod");
    if (!pm_payment_method) {
      return false;
    }
    pm->payment_method = *pm_payment_method;

    const std::string* pm_payment_type =
        pm_item.GetDict().FindString("paymentType");
    if (!pm_payment_type) {
      return false;
    }
    pm->payment_type = *pm_payment_type;

    pm->logo_images = mojom::LogoImages::New();
    if (const auto* logos = pm_item.GetDict().FindDict("logos");
        !ParseMeldLogos(logos, pm->logo_images.get())) {
      return false;
    }
    payment_methods->emplace_back(std::move(pm));
  }

  return true;
}

bool ParseFiatCurrencies(const base::Value& json_value,
                         std::vector<mojom::FiatCurrencyPtr>* fiat_currencies) {
  // Parses results like this:
  // [
  //   {
  //     "currencyCode": "AFN",
  //     "name": "Afghani",
  //     "symbolImageUrl": "https://images-currency.meld.io/fiat/AFN/symbol.png"
  //   }
  // ]

  DCHECK(fiat_currencies);

  if (!json_value.is_list()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is not a list";
    return false;
  }
  for (const auto& fc_item : json_value.GetList()) {
    if (!fc_item.is_dict()) {
      LOG(ERROR)
          << "Invalid response, could not parse JSON, JSON is not a dict";
      return false;
    }
    auto fc = mojom::FiatCurrency::New();
    const std::string* fc_name = fc_item.GetDict().FindString("name");
    if (!fc_name) {
      return false;
    }
    fc->name = *fc_name;

    const std::string* fc_currency_code =
        fc_item.GetDict().FindString("currencyCode");
    if (!fc_currency_code) {
      return false;
    }
    fc->currency_code = *fc_currency_code;

    const std::string* fc_img_url =
        fc_item.GetDict().FindString("symbolImageUrl");
    if (!fc_img_url) {
      return false;
    }
    fc->symbol_image_url = *fc_img_url;

    fiat_currencies->emplace_back(std::move(fc));
  }

  return true;
}

bool ParseCryptoCurrencies(
    const base::Value& json_value,
    std::vector<mojom::CryptoCurrencyPtr>* crypto_currencies) {
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
  DCHECK(crypto_currencies);

  if (!json_value.is_list()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is not a list";
    return false;
  }
  for (const auto& cc_item : json_value.GetList()) {
    if (!cc_item.is_dict()) {
      LOG(ERROR)
          << "Invalid response, could not parse JSON, JSON is not a dict";
      return false;
    }

    auto cc = mojom::CryptoCurrency::New();
    const std::string* cc_name = cc_item.GetDict().FindString("name");
    if (!cc_name) {
      return false;
    }
    cc->name = *cc_name;

    const std::string* cc_code = cc_item.GetDict().FindString("currencyCode");
    if (!cc_code) {
      return false;
    }
    cc->currency_code = *cc_code;

    const std::string* cc_chain_name =
        cc_item.GetDict().FindString("chainName");
    if (!cc_chain_name) {
      return false;
    }
    cc->chain_name = *cc_chain_name;

    const std::string* cc_chain_code =
        cc_item.GetDict().FindString("chainCode");
    if (!cc_chain_code) {
      return false;
    }
    cc->chain_code = *cc_chain_code;

    const std::string* cc_chain_id = cc_item.GetDict().FindString("chainId");
    if (!cc_chain_id) {
      return false;
    }
    cc->chain_id = *cc_chain_id;

    const std::string* cc_contract_addr =
        cc_item.GetDict().FindString("contractAddress");
    if (!cc_contract_addr) {
      return false;
    }
    cc->contract_address = *cc_contract_addr;

    const std::string* cc_img_url =
        cc_item.GetDict().FindString("symbolImageUrl");
    if (!cc_img_url) {
      return false;
    }
    cc->symbol_image_url = *cc_img_url;

    crypto_currencies->emplace_back(std::move(cc));
  }

  return true;
}

bool ParseCountries(const base::Value& json_value,
                    std::vector<mojom::CountryPtr>* countries) {
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
  DCHECK(countries);

  if (!json_value.is_list()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is not a list";
    return false;
  }
  for (const auto& country_item : json_value.GetList()) {
    if (!country_item.is_dict()) {
      LOG(ERROR)
          << "Invalid response, could not parse JSON, JSON is not a dict";
      return false;
    }

    auto country = mojom::Country::New();
    const std::string* cc_name = country_item.GetDict().FindString("name");
    if (!cc_name) {
      return false;
    }
    country->name = *cc_name;

    const std::string* cc_code =
        country_item.GetDict().FindString("countryCode");
    if (!cc_code) {
      return false;
    }
    country->country_code = *cc_code;

    const std::string* cc_img_url =
        country_item.GetDict().FindString("flagImageUrl");
    if (!cc_img_url) {
      return false;
    }
    country->flag_image_url = *cc_img_url;

    countries->emplace_back(std::move(country));
  }

  return true;
}
}  // namespace brave_wallet
