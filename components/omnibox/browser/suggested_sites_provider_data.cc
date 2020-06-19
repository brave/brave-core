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
      "binance.com",
      GURL("https://www.binance.com?ref=17605639"),
      GURL("https://www.binance.com"),
      base::ASCIIToUTF16("binance.com?ref=17605639"))
  }, {
  SuggestedSitesMatch(
      "ftx.com",
      GURL("https://ftx.com/#a=10percentDiscountOnFees"),
      GURL("https://www.ftx.com"),
      base::ASCIIToUTF16("ftx.com/#a=10percentDiscountOnFees"))
  }, {
    SuggestedSitesMatch(
      "bitcoin",
      GURL("https://www.binance.com/en/buy-sell-crypto"
           "?fiat=USD&crypto=BTC&ref=176056396&utm_source=brave"),
      GURL("https://www.binance.com/en/buy-sell-crypto?crypto=BTC"),
      base::ASCIIToUTF16("binance.com/en/buy-sell-crypto?crypto=BTC&ref=17605639"))  // NOLINT
  }, {
    SuggestedSitesMatch(
      "btc",
      GURL("https://www.binance.com/en/buy-sell-crypto"
           "?fiat=USD&crypto=BTC&ref=17605639&utm_source=brave"),
      GURL("https://www.binance.com/en/buy-sell-crypto?crypto=BTC"),
      base::ASCIIToUTF16("binance.com/en/buy-sell-crypto?crypto=BTC&ref=17605639"))  // NOLINT
  }, {
    SuggestedSitesMatch(
      "ethereum",
      GURL("https://www.binance.com/en/buy-sell-crypto"
           "?fiat=USD&crypto=ETH&ref=17605639&utm_source=brave"),
      GURL("https://www.binance.com/en/buy-sell-crypto?crypto=ETH"),
      base::ASCIIToUTF16("binance.com/en/buy-sell-crypto?crypto=ETH&ref=17605639"))  // NOLINT
  }, {
    SuggestedSitesMatch(
      "eth",
      GURL("https://www.binance.com/en/buy-sell-crypto"
           "?fiat=USD&crypto=ETH&ref=17605639&utm_source=brave"),
      GURL("https://www.binance.com/en/buy-sell-crypto?crypto=ETH"),
      base::ASCIIToUTF16("binance.com/en/buy-sell-crypto?crypto=ETH&ref=17605639"))  // NOLINT
  }, {
    SuggestedSitesMatch(
      "litecoin",
      GURL("https://www.binance.com/en/buy-sell-crypto"
           "?fiat=USD&crypto=LTC&ref=17605639&utm_source=brave"),
      GURL("https://www.binance.com/en/buy-sell-crypto?crypto=LTC"),
      base::ASCIIToUTF16("binance.com/en/buy-sell-crypto?crypto=LTC&ref=17605639"))  // NOLINT
  }, {
    SuggestedSitesMatch(
      "ltc",
      GURL("https://www.binance.com/en/buy-sell-crypto"
           "?fiat=USD&crypto=LTC&ref=17605639&utm_source=brave"),
      GURL("https://www.binance.com/en/buy-sell-crypto?crypto=LTC"),
      base::ASCIIToUTF16("binance.com/en/buy-sell-crypto?crypto=LTC&ref=17605639"))  // NOLINT
  }, {
    SuggestedSitesMatch(
      "bnb",
      GURL("https://www.binance.com/en/buy-sell-crypto"
           "?fiat=USD&crypto=BNB&ref=17605639&utm_source=brave"),
      GURL("https://www.binance.com/en/buy-sell-crypto?crypto=BNB"),
      base::ASCIIToUTF16("binance.com/en/buy-sell-crypto?crypto=BNB&ref=17605639"))  // NOLINT
  }, {
    SuggestedSitesMatch(
      "binance.us",
      GURL("https://www.binance.us?ref=35089877"),
      GURL("https://www.binance.us"),
      base::ASCIIToUTF16("binance.us?ref=35089877"))
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
  }, {
    SuggestedSitesMatch(
      "coinbase.com/join",
      GURL("https://www.coinbase.com/join/sezc_n"),
      GURL("https://www.coinbase.com/join"),
      base::ASCIIToUTF16("coinbase.com/join/sezc_n"))
  }, {
    SuggestedSitesMatch(
      "ledger.com/pages/ledger-nano-x",
      GURL("https://shop.ledger.com/pages/ledger-nano-x?r=6c8c"),
      GURL("https://shop.ledger.com/pages/ledger-nano-x"),
      base::ASCIIToUTF16("shop.ledger.com/pages/ledger-nano-x?r=6c8c"))
  }, {
    SuggestedSitesMatch(
      "trezor.io/product/trezor-one-metallic",
      GURL("https://shop.trezor.io/product/trezor-one-metallic"
           "?offer_id=24&aff_id=3494"),
      GURL("https://shop.trezor.io/product/trezor-one-metallic"),
      base::ASCIIToUTF16("shop.trezor.io/product/trezor-one-metallic"
                         "?offer_id=24&aff_id=3494"))
  },
  };

  return suggested_sites;
}
