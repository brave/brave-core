/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BINANCE_BROWSER_BINANCE_JSON_PARSER_H_
#define BRAVE_COMPONENTS_BINANCE_BROWSER_BINANCE_JSON_PARSER_H_

#include <map>
#include <string>
#include <vector>

class BinanceJSONParser {
 public:
  static bool GetTokensFromJSON(const std::string& json,
                                std::string *value, std::string type);
  static bool GetAccountBalancesFromJSON(const std::string& json,
                                        std::map<std::string, std::string>*);
  static bool GetQuoteIDFromJSON(const std::string& json,
                                 std::string *quote_id);
  static bool GetTickerPriceFromJSON(const std::string& json,
                                     std::string* symbol_pair_price);
  static bool GetTickerVolumeFromJSON(const std::string& json,
                                      std::string* symbol_pair_volume);
  static bool GetDepositInfoFromJSON(const std::string& json,
                                     std::string* address,
                                     std::string *url);
  static bool GetQuoteInfoFromJSON(const std::string& json,
                                   std::string* quote_id,
                                   std::string* quote_price,
                                   std::string* total_fee,
                                   std::string* total_amount);
  static bool GetConfirmStatusFromJSON(const std::string& json,
                                       std::string *success_status);
  static bool GetConvertAssetsFromJSON(const std::string& json,
    std::map<std::string, std::vector<std::string>>* assets);
};

#endif  // BRAVE_COMPONENTS_BINANCE_BROWSER_BINANCE_JSON_PARSER_H_
