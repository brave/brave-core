// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

export const enum types {
  CREATE_WALLET = '@@rewards/CREATE_WALLET',
  DISMISS_NOTIFICATION = '@@rewards/DISMISS_NOTIFICATION',
  ON_ADS_ENABLED = '@@rewards/ON_ADS_ENABLED',
  ON_ADS_ESTIMATED_EARNINGS = '@@rewards/ON_ADS_ESTIMATED_EARNINGS',
  ON_ENABLED_MAIN = '@@rewards/ON_ENABLED_MAIN',
  ON_WALLET_INITIALIZED = '@@rewards/ON_WALLET_INITIALIZED',
  ON_BALANCE_REPORT = '@@rewards/ON_BALANCE_REPORT',
  ON_PROMOTIONS = '@@rewards/ON_PROMOTIONS',
  ON_PROMOTION_FINISH = '@@rewards/ON_PROMOTION_FINISH',
  ON_BALANCE = '@@rewards/ON_BALANCE',
  ON_WALLET_EXISTS = '@@rewards/ON_WALLET_EXISTS',
  SET_INITIAL_REWARDS_DATA = '@@rewards/SET_INITIAL_REWARDS_DATA',
  SET_PRE_INITIAL_REWARDS_DATA = '@@rewards/SET_PRE_INITIAL_REWARDS_DATA',
  ON_WIDGET_POSITION_CHANGED = '@@rewards/ON_WIDGET_POSITION_CHANGED',
  SET_ONLY_ANON_WALLET = '@@rewards/SET_ONLY_ANON_WALLET',
  ON_COMPLETE_RESET = '@@rewards/ON_COMPLETE_RESET'
}
