// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export const enum types {
  DISMISS_NOTIFICATION = '@@rewards/DISMISS_NOTIFICATION',
  ON_ADS_ENABLED = '@@rewards/ON_ADS_ENABLED',
  ON_ADS_ACCOUNT_STATEMENT = '@@rewards/ON_ADS_ACCOUNT_STATEMENT',
  ON_WALLET_INITIALIZED = '@@rewards/ON_WALLET_INITIALIZED',
  ON_BALANCE_REPORT = '@@rewards/ON_BALANCE_REPORT',
  ON_BALANCE = '@@rewards/ON_BALANCE',
  SET_INITIAL_REWARDS_DATA = '@@rewards/SET_INITIAL_REWARDS_DATA',
  SET_PRE_INITIAL_REWARDS_DATA = '@@rewards/SET_PRE_INITIAL_REWARDS_DATA',
  ON_WIDGET_POSITION_CHANGED = '@@rewards/ON_WIDGET_POSITION_CHANGED',
  ON_COMPLETE_RESET = '@@rewards/ON_COMPLETE_RESET'
}
