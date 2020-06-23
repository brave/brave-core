/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/suggested_sites_provider.h"

#include "base/strings/utf_string_conversions.h"

const std::vector<SuggestedSitesMatch>&
SuggestedSitesProvider::GetSuggestedSites() {
  static const std::vector<SuggestedSitesMatch> suggested_sites = {
  {
    SuggestedSitesMatch(
      "bitcoin",
      GURL("https://www.binance.com/en/buy-sell-crypto"
           "?fiat=USD&crypto=BTC&ref=39346846&utm_source=brave"),
      GURL("https://www.binance.com/en/buy-sell-crypto?crypto=BTC"),
      base::ASCIIToUTF16("binance.com/en/buy-sell-crypto?crypto=BTC&ref=39346846"))  // NOLINT
  }, {
    SuggestedSitesMatch(
      "btc",
      GURL("https://www.binance.com/en/buy-sell-crypto"
           "?fiat=USD&crypto=BTC&ref=39346846&utm_source=brave"),
      GURL("https://www.binance.com/en/buy-sell-crypto?crypto=BTC"),
      base::ASCIIToUTF16("binance.com/en/buy-sell-crypto?crypto=BTC&ref=39346846"))  // NOLINT
  }, {
    SuggestedSitesMatch(
      "ethereum",
      GURL("https://www.binance.com/en/buy-sell-crypto"
           "?fiat=USD&crypto=ETH&ref=39346846&utm_source=brave"),
      GURL("https://www.binance.com/en/buy-sell-crypto?crypto=ETH"),
      base::ASCIIToUTF16("binance.com/en/buy-sell-crypto?crypto=ETH&ref=39346846"))  // NOLINT
  }, {
    SuggestedSitesMatch(
      "eth",
      GURL("https://www.binance.com/en/buy-sell-crypto"
           "?fiat=USD&crypto=ETH&ref=39346846&utm_source=brave"),
      GURL("https://www.binance.com/en/buy-sell-crypto?crypto=ETH"),
      base::ASCIIToUTF16("binance.com/en/buy-sell-crypto?crypto=ETH&ref=39346846"))  // NOLINT
  }, {
    SuggestedSitesMatch(
      "litecoin",
      GURL("https://www.binance.com/en/buy-sell-crypto"
           "?fiat=USD&crypto=LTC&ref=39346846&utm_source=brave"),
      GURL("https://www.binance.com/en/buy-sell-crypto?crypto=LTC"),
      base::ASCIIToUTF16("binance.com/en/buy-sell-crypto?crypto=LTC&ref=39346846"))  // NOLINT
  }, {
    SuggestedSitesMatch(
      "ltc",
      GURL("https://www.binance.com/en/buy-sell-crypto"
           "?fiat=USD&crypto=LTC&ref=39346846&utm_source=brave"),
      GURL("https://www.binance.com/en/buy-sell-crypto?crypto=LTC"),
      base::ASCIIToUTF16("binance.com/en/buy-sell-crypto?crypto=LTC&ref=39346846"))  // NOLINT
  }, {
    SuggestedSitesMatch(
      "bnb",
      GURL("https://www.binance.com/en/buy-sell-crypto"
           "?fiat=USD&crypto=BNB&ref=39346846&utm_source=brave"),
      GURL("https://www.binance.com/en/buy-sell-crypto?crypto=BNB"),
      base::ASCIIToUTF16("binance.com/en/buy-sell-crypto?crypto=BNB&ref=39346846"))  // NOLINT
  }, {
    SuggestedSitesMatch(
      "bitcoin",
      GURL("https://www.binance.us/en/buy-sell-crypto"
           "?fiat=USD&crypto=BTC&ref=35089877&utm_source=brave"),
      GURL("https://www.binance.us/en/buy-sell-crypto?crypto=BTC"),
      base::ASCIIToUTF16("binance.us/en/buy-sell-crypto?crypto=BTC&ref=35089877"))  // NOLINT
  }, {
    SuggestedSitesMatch(
      "btc",
      GURL("https://www.binance.us/en/buy-sell-crypto"
           "?fiat=USD&crypto=BTC&ref=35089877&utm_source=brave"),
      GURL("https://www.binance.us/en/buy-sell-crypto?crypto=BTC"),
      base::ASCIIToUTF16("binance.us/en/buy-sell-crypto?crypto=BTC&ref=35089877"))  // NOLINT
  }, {
    SuggestedSitesMatch(
     "ethereum",
      GURL("https://www.binance.us/en/buy-sell-crypto"
           "?fiat=USD&crypto=ETH&ref=35089877&utm_source=brave"),
      GURL("https://www.binance.us/en/buy-sell-crypto?crypto=ETH"),
      base::ASCIIToUTF16("binance.us/en/buy-sell-crypto?crypto=ETH&ref=35089877"))  // NOLINT
  }, {
    SuggestedSitesMatch(
      "eth",
      GURL("https://www.binance.us/en/buy-sell-crypto"
           "?fiat=USD&crypto=ETH&ref=35089877&utm_source=brave"),
      GURL("https://www.binance.us/en/buy-sell-crypto?crypto=ETH"),
      base::ASCIIToUTF16("binance.us/en/buy-sell-crypto?crypto=ETH&ref=35089877"))  // NOLINT
  }, {
    SuggestedSitesMatch(
      "bnb",
      GURL("https://www.binance.us/en/buy-sell-crypto"
           "?fiat=USD&crypto=BNB&ref=35089877&utm_source=brave"),
      GURL("https://www.binance.us/en/buy-sell-crypto?crypto=BNB"),
      base::ASCIIToUTF16("binance.us/en/buy-sell-crypto?crypto=BNB&ref=35089877"))  // NOLINT
  }};

  return suggested_sites;
}
