// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

export const enum types {
  ON_BINANCE_USER_TLD = '@@binance/ON_BINANCE_USER_TLD',
  ON_BINANCE_USER_LOCALE = '@@binance/ON_BINANCE_USER_LOCALE',
  SET_INITIAL_ASSET = '@@binance/SET_INITIAL_ASSET',
  SET_INITIAL_FIAT = '@@binance/SET_INITIAL_FIAT',
  SET_INITIAL_AMOUNT = '@@binance/SET_INITIAL_AMOUNT',
  SET_USER_TLD_AUTO_SET = '@@binance/SET_USER_TLD_AUTO_SET',
  SET_BINANCE_SUPPORTED = '@@binance/SET_BINANCE_SUPPORTED',
  ON_BINANCE_CLIENT_URL = '@@binance/ON_BINANCE_CLIENT_URL',
  ON_VALID_BINANCE_AUTH_CODE = '@@binance/ON_VALID_BINANCE_AUTH_CODE',
  SET_HIDE_BALANCE = '@@binance/SET_HIDE_BALANCE',
  CONNECT_TO_BINANCE = '@@binance/CONNECT_TO_BINANCE',
  DISCONNECT_BINANCE = '@@binance/DISCONNECT_BINANCE',
  ON_ASSETS_BALANCE_INFO = '@@binance/ON_ASSETS_BALANCE_INFO',
  ON_ASSET_DEPOSIT_INFO = '@@binance/ON_ASSET_DEPOSIT_INFO',
  ON_DEPOSIT_QR_FOR_ASSET = '@@binance/ON_DEPOSIT_QR_FOR_ASSET',
  ON_CONVERTABLE_ASSETS = '@@binance/ON_CONVERTABLE_ASSETS',
  SET_DISCONNECT_IN_PROGRESS = '@@binance/SET_DISCONNECT_IN_PROGRESS',
  SET_AUTH_INVALID = '@@binance/SET_AUTH_INVALID',
  SET_SELECTED_VIEW = '@@binance/SET_SELECTED_VIEW',
  SET_DEPOSIT_INFO_SAVED = '@@binance/SET_DEPOSIT_INFO_SAVED'
}
