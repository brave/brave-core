/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CRYPTO_DOT_COM_COMMON_CONSTANTS_H_
#define BRAVE_COMPONENTS_CRYPTO_DOT_COM_COMMON_CONSTANTS_H_

constexpr char kCryptoDotComAuthURL[] =
    "https://auth.crypto.com/exchange/widget/sign_in";
constexpr char kCryptoDotComGetAccountBalanceURL[] =
    "https://crypto.com/fe-ex-api/widget/get-account-summary";
constexpr char kCryptoDotComGetDepositAddressURL[] =
    "https://crypto.com/fe-ex-api/widget/get-deposit-address";
constexpr char kCryptoDotComCreateMarketOrderURL[] =
    "https://crypto.com/fe-ex-api/widget/create-order";
constexpr char kCryptoDotComGetNewsEventsURL[] =
    "https://crypto.com/fe-ex-api/widget/get-events";
constexpr char kCryptoDotComDisconnectURL[] =
    "https://crypto.com/fe-ex-api/widget/disconnect";
// These empty value is passed when corresponding apis get failed.
constexpr char kEmptyAccountBalances[] = R"(
    {
      "total_balance": "0",
      "accounts": [
        {
          "stake": "0",
          "balance": "0",
          "available": "0",
          "currency": "0",
          "currency_decimals": 0,
          "order": "0"
        }
      ]
    })";

constexpr char kEmptyNewsEvents[] = R"(
    {
      "events": [
        {
          "content": "",
          "redirect_title": "",
          "redirect_url": "",
          "updated_at": ""
        }
      ]
    })";

constexpr char kEmptyDepositAddress[] = R"(
    {
      "address": "",
      "qr_code": "",
      "currency": ""
    })";

#endif  // BRAVE_COMPONENTS_CRYPTO_DOT_COM_COMMON_CONSTANTS_H_
