/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/asset_ratio_response_parser.h"
#include <optional>
#include <vector>

#include "absl/types/optional.h"
#include "base/logging.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_wallet/api/asset_ratio.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace {

bool ParseMeldLogos(const base::Value::Dict* logos,
                    std::vector<std::string>& logo_images) {
  if (!logos) {
    return false;
  }
  if (const auto* dark_logo = logos->FindString("dark")) {
    logo_images.push_back(*dark_logo);
  }
  if (const auto* dark_short_logo = logos->FindString("darkShort")) {
    logo_images.push_back(*dark_short_logo);
  }
  if (const auto* light_logo = logos->FindString("light")) {
    logo_images.push_back(*light_logo);
  }
  if (const auto* light_short_logo = logos->FindString("lightShort")) {
    logo_images.push_back(*light_short_logo);
  }

  return true;
}

}  //  namespace

namespace brave_wallet {

std::optional<std::string> ParseSardineAuthToken(
    const base::Value& json_value) {
  // Parses results like this:
  // {
  //   "clientToken":"74618e17-a537-4f5d-ab4d-9916739560b1",
  //   "expiresAt":"2022-07-25T19:59:57Z"
  //   "name": "brave-core",
  // }

  if (!json_value.is_dict()) {
    VLOG(0) << "Invalid response, JSON is not a dict";
    return std::nullopt;
  }

  const std::string* auth_token =
      json_value.GetDict().FindString("clientToken");
  if (!auth_token) {
    return std::nullopt;
  }

  return *auth_token;
}

bool ParseAssetPrice(const base::Value& json_value,
                     const std::vector<std::string>& from_assets,
                     const std::vector<std::string>& to_assets,
                     std::vector<mojom::AssetPricePtr>* values) {
  // Parses results like this:
  // /v2/relative/provider/coingecko/bat,chainlink/btc,usd/1w
  // {
  //  "payload": {
  //    "chainlink": {
  //      "btc": 0.00063075,
  //      "usd": 29.17,
  //      "btc_timeframe_change": -0.9999742658279261,
  //      "usd_timeframe_change": 0.1901162098990581
  //    },
  //    "bat": {
  //      "btc": 1.715e-05,
  //      "usd": 0.793188,
  //      "btc_timeframe_change": -0.9999993002916352,
  //      "usd_timeframe_change": -0.9676384677306338
  //    }
  //  },
  //  "lastUpdated": "2021-08-16T15:45:11.901Z"
  // }

  DCHECK(values);

  if (!json_value.is_dict()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is not a dict";
    return false;
  }

  const auto& response_dict = json_value.GetDict();
  const auto* payload = response_dict.FindDict("payload");
  if (!payload) {
    return false;
  }

  for (const std::string& from_asset : from_assets) {
    const auto* from_asset_dict = payload->FindDictByDottedPath(from_asset);
    if (!from_asset_dict) {
      return false;
    }

    for (const std::string& to_asset : to_assets) {
      auto asset_price = mojom::AssetPrice::New();
      asset_price->from_asset = from_asset;
      asset_price->to_asset = to_asset;

      std::optional<double> to_price =
          from_asset_dict->FindDoubleByDottedPath(to_asset);
      if (!to_price) {
        return false;
      }
      asset_price->price = base::NumberToString(*to_price);
      std::string to_asset_timeframe_key =
          base::StringPrintf("%s_timeframe_change", to_asset.c_str());
      std::optional<double> to_timeframe_change =
          from_asset_dict->FindDoubleByDottedPath(to_asset_timeframe_key);
      if (!to_timeframe_change) {
        return false;
      }
      asset_price->asset_timeframe_change =
          base::NumberToString(*to_timeframe_change);

      values->push_back(std::move(asset_price));
    }
  }

  return true;
}

bool ParseAssetPriceHistory(const base::Value& json_value,
                            std::vector<mojom::AssetTimePricePtr>* values) {
  DCHECK(values);

  // {  "payload":
  //   {
  //     "prices":[[1622733088498,0.8201346624954003],[1622737203757,0.8096978545029869]],
  //     "market_caps":[[1622733088498,1223507820.383275],[1622737203757,1210972881.4928021]],
  //     "total_volumes":[[1622733088498,163426828.00299588],[1622737203757,157618689.0971025]]
  //   }
  // }

  if (!json_value.is_dict()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is not a dict";
    return false;
  }

  const auto& response_dict = json_value.GetDict();
  const auto* payload = response_dict.FindDict("payload");
  if (!payload) {
    return false;
  }

  const auto* prices_list = payload->FindList("prices");
  if (!prices_list) {
    return false;
  }

  for (const auto& date_price_list_it : *prices_list) {
    const auto* date_price_list = date_price_list_it.GetIfList();
    if (!date_price_list) {
      return false;
    }
    auto it = date_price_list->begin();
    const auto& date_value = *it;
    const auto& price_value = *(++it);

    // Check whether date_value is convertible to a double first.
    if (!date_value.is_double() && !date_value.is_int()) {
      return false;
    }
    double date_dbl = date_value.GetDouble();

    // Check whether price_value is convertible to a double first.
    if (!price_value.is_double() && !price_value.is_int()) {
      return false;
    }
    double price = price_value.GetDouble();

    base::Time date = base::Time::FromMillisecondsSinceUnixEpoch(date_dbl);
    auto asset_time_price = mojom::AssetTimePrice::New();
    asset_time_price->date =
        base::Milliseconds(date.InMillisecondsSinceUnixEpoch());
    asset_time_price->price = base::NumberToString(price);
    values->push_back(std::move(asset_time_price));
  }

  return true;
}

std::optional<std::vector<mojom::CoinMarketPtr>> ParseCoinMarkets(
    const base::Value& json_value) {
  auto coin_market_data = api::asset_ratio::CoinMarket::FromValue(json_value);

  if (!coin_market_data) {
    return std::nullopt;
  }

  std::vector<mojom::CoinMarketPtr> values;
  for (const auto& payload : coin_market_data->payload) {
    auto coin_market = mojom::CoinMarket::New();
    coin_market->id = payload.id;
    coin_market->symbol = payload.symbol;
    coin_market->name = payload.name;
    coin_market->image = payload.image;
    coin_market->market_cap = payload.market_cap;
    coin_market->market_cap_rank = payload.market_cap_rank;
    coin_market->current_price = payload.current_price;
    coin_market->price_change_24h = payload.price_change_24h;
    coin_market->price_change_percentage_24h =
        payload.price_change_percentage_24h;
    coin_market->total_volume = payload.total_volume;
    values.push_back(std::move(coin_market));
  }
  return values;
}

std::optional<std::string> ParseStripeBuyURL(const base::Value& json_value) {
  // Parses results like this:
  // {
  //   "url": "https://crypto.link.com?session_hash=abcdefgh"
  // }
  auto stripe_buy_url_response =
      api::asset_ratio::StripeBuyURLResponse::FromValue(json_value);

  if (!stripe_buy_url_response) {
    return std::nullopt;
  }

  return stripe_buy_url_response->url;
}

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

    if (const auto* logos = sp_item.GetDict().FindDict("logos");
        !ParseMeldLogos(logos, sp->logo_images)) {
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

    auto quote_er = item.GetDict().FindDouble("exchangeRate");
    if (!quote_er) {
      return false;
    }
    quote->exchange_rate = *quote_er;

    auto quote_amount = item.GetDict().FindDouble("sourceAmount");
    if (!quote_amount) {
      return false;
    }
    quote->source_amount = *quote_amount;

    auto quote_amount_without_fee =
        item.GetDict().FindDouble("sourceAmountWithoutFees");
    if (!quote_amount_without_fee) {
      return false;
    }
    quote->source_amount_without_fee = *quote_amount_without_fee;

    auto quote_total_fee = item.GetDict().FindDouble("totalFee");
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

    auto quote_dest_amount = item.GetDict().FindDouble("destinationAmount");
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

    if (const auto* logos = pm_item.GetDict().FindDict("logos");
        !ParseMeldLogos(logos, pm->logo_images)) {
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
