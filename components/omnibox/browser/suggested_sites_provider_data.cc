/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/suggested_sites_provider.h"

#include "base/strings/utf_string_conversions.h"

std::map<std::string, SuggestedSitesMatch>
SuggestedSitesProvider::suggested_sites_ = {
  {
    "binance.com", SuggestedSitesMatch(
      GURL("https://www.binance.com?ref=39346846"),
      GURL("https://www.binance.com"),
      base::ASCIIToUTF16("binance.com?ref=39346846"),
      true)
  }, {
    "bitcoin", SuggestedSitesMatch(
      GURL("https://www.binance.com/buy-sell-crypto"
           "?fiat=USD&crypto=BTC&ref=39346846&utm_source=brave"),
      GURL("https://www.binance.com/buy-sell-crypto?crypto=BTC"),
      base::ASCIIToUTF16("binance.com/buy-sell-crypto?crypto=BTC&ref=39346846"),
      false)
  }, {
    "btc", SuggestedSitesMatch(
      GURL("https://www.binance.com/buy-sell-crypto"
           "?fiat=USD&crypto=BTC&ref=39346846&utm_source=brave"),
      GURL("https://www.binance.com/buy-sell-crypto?crypto=BTC"),
      base::ASCIIToUTF16("binance.com/buy-sell-crypto?crypto=BTC&ref=39346846"),
      false)
  }, {
    "ethereum", SuggestedSitesMatch(
      GURL("https://www.binance.com/buy-sell-crypto"
           "?fiat=USD&crypto=ETH&ref=39346846&utm_source=brave"),
      GURL("https://www.binance.com/buy-sell-crypto?crypto=ETH"),
      base::ASCIIToUTF16("binance.com/buy-sell-crypto?crypto=ETH&ref=39346846"),
      false)
  }, {
    "eth", SuggestedSitesMatch(
      GURL("https://www.binance.com/buy-sell-crypto"
           "?fiat=USD&crypto=ETH&ref=39346846&utm_source=brave"),
      GURL("https://www.binance.com/buy-sell-crypto?crypto=ETH"),
      base::ASCIIToUTF16("binance.com/buy-sell-crypto?crypto=ETH&ref=39346846"),
      false)
  }, {
    "litecoin", SuggestedSitesMatch(
      GURL("https://www.binance.com/buy-sell-crypto"
           "?fiat=USD&crypto=LTC&ref=39346846&utm_source=brave"),
      GURL("https://www.binance.com/buy-sell-crypto?crypto=LTC"),
      base::ASCIIToUTF16("binance.com/buy-sell-crypto?crypto=LTC&ref=39346846"),
      false)
  }, {
    "ltc", SuggestedSitesMatch(
      GURL("https://www.binance.com/buy-sell-crypto"
           "?fiat=USD&crypto=LTC&ref=39346846&utm_source=brave"),
      GURL("https://www.binance.com/buy-sell-crypto?crypto=LTC"),
      base::ASCIIToUTF16("binance.com/buy-sell-crypto?crypto=LTC&ref=39346846"),
      false)
  }, {
    "bnb", SuggestedSitesMatch(
      GURL("https://www.binance.com/buy-sell-crypto"
           "?fiat=USD&crypto=BNB&ref=39346846&utm_source=brave"),
      GURL("https://www.binance.com/buy-sell-crypto?crypto=BNB"),
      base::ASCIIToUTF16("binance.com/buy-sell-crypto?crypto=BNB&ref=39346846"),
      false)
  }, {
    "binance.us", SuggestedSitesMatch(
      GURL("https://www.binance.us?ref=35089877"),
      GURL("https://www.binance.us"),
      base::ASCIIToUTF16("binance.us?ref=35089877"),
      true)
  }, {
    "bitcoin", SuggestedSitesMatch(
      GURL("https://www.binance.us/buy-sell-crypto"
           "?fiat=USD&crypto=BTC&ref=35089877&utm_source=brave"),
      GURL("https://www.binance.us/buy-sell-crypto?crypto=BTC"),
      base::ASCIIToUTF16("binance.us/buy-sell-crypto?crypto=BTC&ref=35089877"),
      false)
  }, {
    "btc", SuggestedSitesMatch(
      GURL("https://www.binance.us/buy-sell-crypto"
           "?fiat=USD&crypto=BTC&ref=35089877&utm_source=brave"),
      GURL("https://www.binance.us/buy-sell-crypto?crypto=BTC"),
      base::ASCIIToUTF16("binance.us/buy-sell-crypto?crypto=BTC&ref=35089877"),
      false)
  }, {
    "ethereum", SuggestedSitesMatch(
      GURL("https://www.binance.us/buy-sell-crypto"
           "?fiat=USD&crypto=ETH&ref=35089877&utm_source=brave"),
      GURL("https://www.binance.us/buy-sell-crypto?crypto=ETH"),
      base::ASCIIToUTF16("binance.us/buy-sell-crypto?crypto=ETH&ref=35089877"),
      false)
  }, {
    "eth", SuggestedSitesMatch(
      GURL("https://www.binance.us/buy-sell-crypto"
           "?fiat=USD&crypto=ETH&ref=35089877&utm_source=brave"),
      GURL("https://www.binance.us/buy-sell-crypto?crypto=ETH"),
      base::ASCIIToUTF16("binance.us/buy-sell-crypto?crypto=ETH&ref=35089877"),
      false)
  }, {
    "litecoin", SuggestedSitesMatch(
      GURL("https://www.binance.us/buy-sell-crypto"
           "?fiat=USD&crypto=LTC&ref=35089877&utm_source=brave"),
      GURL("https://www.binance.us/buy-sell-crypto?crypto=LTC"),
      base::ASCIIToUTF16("binance.us/buy-sell-crypto?crypto=LTC&ref=35089877"),
      false)
  }, {
    "ltc", SuggestedSitesMatch(
      GURL("https://www.binance.us/buy-sell-crypto"
           "?fiat=USD&crypto=LTC&ref=35089877&utm_source=brave"),
      GURL("https://www.binance.us/buy-sell-crypto?crypto=LTC"),
      base::ASCIIToUTF16("binance.us/buy-sell-crypto?crypto=LTC&ref=35089877"),
      false)
  }, {
    "bnb", SuggestedSitesMatch(
      GURL("https://www.binance.us/buy-sell-crypto"
           "?fiat=USD&crypto=BNB&ref=35089877&utm_source=brave"),
      GURL("https://www.binance.us/buy-sell-crypto?crypto=BNB"),
      base::ASCIIToUTF16("binance.us/buy-sell-crypto?crypto=BNB&ref=35089877"),
      false)
  }, {
    "coinbase.com/join", SuggestedSitesMatch(
      GURL("https://www.coinbase.com/join/sezc_n"),
      GURL("https://www.coinbase.com/join"),
      base::ASCIIToUTF16("coinbase.com/join/sezc_n"),
      true)
  }, {
    "ledger.com/pages/ledger-nano-x", SuggestedSitesMatch(
      GURL("https://shop.ledger.com/pages/ledger-nano-x?r=0ba5d7199327"),
      GURL("https://shop.ledger.com/pages/ledger-nano-x"),
      base::ASCIIToUTF16("shop.ledger.com/pages/ledger-nano-x?r=0ba5d7199327"),
      true)
  }, {
    "trezor.io/product/trezor-one-metallic", SuggestedSitesMatch(
      GURL("https://shop.trezor.io/product/trezor-one-metallic"
           "?offer_id=24&aff_id=3494"),
      GURL("https://shop.trezor.io/product/trezor-one-metallic"),
      base::ASCIIToUTF16("shop.trezor.io/product/trezor-one-metallic"
                         "?offer_id=24&aff_id=3494"),
      true)
  },
};
