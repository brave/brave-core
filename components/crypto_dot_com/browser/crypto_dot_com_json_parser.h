/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CRYPTO_DOT_COM_BROWSER_CRYPTO_DOT_COM_JSON_PARSER_H_
#define BRAVE_COMPONENTS_CRYPTO_DOT_COM_BROWSER_CRYPTO_DOT_COM_JSON_PARSER_H_

#include <map>
#include <string>
#include <vector>

#include "brave/components/crypto_dot_com/browser/crypto_dot_com_service.h"

class CryptoDotComJSONParser {
 public:
  static bool GetTickerInfoFromJSON(const std::string& json,
      CryptoDotComTickerInfo* info);
  static bool GetChartDataFromJSON(const std::string& json,
      CryptoDotComChartData* data);
  static bool GetPairsFromJSON(const std::string& json,
      CryptoDotComSupportedPairs* pairs);
  static bool GetRankingsFromJSON(const std::string& json,
      CryptoDotComAssetRankings* rankings);
 private:
  static void CalculateAssetVolume(const double v,
      const double h,
      const double l,
      std::string* volume);
};

#endif  // BRAVE_COMPONENTS_CRYPTO_DOT_COM_BROWSER_CRYPTO_DOT_COM_JSON_PARSER_H_
