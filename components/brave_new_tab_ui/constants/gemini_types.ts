// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

export const enum types {
  ON_GEMINI_CLIENT_URL = '@@gemini/ON_GEMINI_CLIENT_URL',
  ON_VALID_GEMINI_AUTH_CODE = '@@gemini/ON_VALID_GEMINI_AUTH_CODE',
  CONNECT_TO_GEMINI = '@@gemini/CONNECT_TO_GEMINI',
  SET_GEMINI_TICKER_PRICE = '@@gemini/SET_GEMINI_TICKER_PRICE',
  SET_SELECTED_VIEW = '@@gemini/SET_SELECTED_VIEW',
  SET_HIDE_BALANCE = '@@gemini/SET_HIDE_BALANCE',
  SET_ACCOUNT_BALANCES = '@@gemini/SET_ACCOUNT_BALANCES',
  SET_ASSET_ADDRESS = '@@gemini/SET_ASSET_ADDRESS',
  DISCONNECT_GEMINI = '@@gemini/DISCONNECT_GEMINI',
  SET_DISCONNECT_IN_PROGRESS = '@@gemini/SET_DISCONNECT_IN_PROGRESS',
  SET_AUTH_INVALID = '@@/SET_AUTH_INVALID'
}
