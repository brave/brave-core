/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/asset_ratio_response_parser.h"

#include "base/check.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_wallet/api/asset_ratio.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_wallet {

namespace {

std::optional<mojom::Gate3CacheStatus> GetCacheStatusFromString(
    const std::string& cache_status) {
  std::string lowercase_status = base::ToUpperASCII(cache_status);
  if (lowercase_status == "HIT") {
    return mojom::Gate3CacheStatus::kHit;
  } else if (lowercase_status == "MISS") {
    return mojom::Gate3CacheStatus::kMiss;
  }
  return std::nullopt;
}

std::optional<mojom::AssetPriceSource> GetAssetPriceSourceFromString(
    const std::string& source) {
  std::string uppercase_source = base::ToUpperASCII(source);
  if (uppercase_source == "COINGECKO") {
    return mojom::AssetPriceSource::kCoingecko;
  } else if (uppercase_source == "JUPITER") {
    return mojom::AssetPriceSource::kJupiter;
  }
  return std::nullopt;
}

}  // namespace

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

std::vector<mojom::AssetPricePtr> ParseAssetPrices(
    const base::Value& json_value) {
  // Parses results from the new /api/pricing/v1/getPrices endpoint:
  // [
  //   {
  //     "coin_type": "ETH",
  //     "chain_id": "0x1",
  //     "address": "0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
  //     "price": "0.55393",
  //     "percentge_change_24h": "0.1",
  //     "vs_currency": "USD",
  //     "cache_status": "HIT",
  //     "source": "coingecko"
  //   }
  // ]

  if (!json_value.is_list()) {
    LOG(ERROR) << "Invalid response, expected array";
    return {};
  }

  const auto& response_list = json_value.GetList();

  std::vector<mojom::AssetPricePtr> prices;
  for (const auto& item : response_list) {
    if (!item.is_dict()) {
      LOG(ERROR) << "Invalid response, expected dict";
      continue;
    }

    const auto& response_dict = item.GetDict();
    const std::string* coin_type_str = response_dict.FindString("coin_type");
    const std::string* chain_id = response_dict.FindString("chain_id");
    const std::string* address = response_dict.FindString("address");
    const std::string* price = response_dict.FindString("price");
    const std::string* vs_currency_str =
        response_dict.FindString("vs_currency");
    const std::string* cache_status_str =
        response_dict.FindString("cache_status");
    const std::string* source_str = response_dict.FindString("source");
    const std::string* percentage_change_24h_str =
        response_dict.FindString("percentage_change_24h");

    if (!coin_type_str || !price || !vs_currency_str || !cache_status_str ||
        !source_str) {
      continue;
    }

    auto coin_type = GetCoinTypeFromString(*coin_type_str);
    if (!coin_type) {
      continue;
    }

    auto cache_status = GetCacheStatusFromString(*cache_status_str);
    if (!cache_status) {
      continue;
    }

    auto source = GetAssetPriceSourceFromString(*source_str);
    if (!source) {
      continue;
    }

    auto asset_price = mojom::AssetPrice::New();
    asset_price->coin_type = *coin_type;
    if (chain_id) {
      asset_price->chain_id = *chain_id;
    }
    if (address) {
      asset_price->address = *address;
    }
    asset_price->price = *price;
    asset_price->vs_currency = *vs_currency_str;
    asset_price->cache_status = *cache_status;
    asset_price->source = *source;
    asset_price->percentage_change_24h =
        percentage_change_24h_str ? *percentage_change_24h_str : "";

    prices.push_back(std::move(asset_price));
  }

  return prices;
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

}  // namespace brave_wallet
