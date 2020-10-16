// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

export const enum types {
  MARKETS_REQUESTED = '@@cryptoDotCom/MARKETS_REQUESTED',
  MARKETS_RECEIVED = '@@cryptoDotCom/MARKETS_RECEIVED',
  ALL_ASSETS_DETAILS_REQUESTED = '@@cryptoDotCom/ALL_ASSETS_DETAILS_REQUESTED',
  ALL_ASSETS_DETAILS_RECEIVED = '@@cryptoDotCom/ALL_ASSETS_DETAILS_RECEIVED',
  ON_REFRESH_DATA = '@@cryptoDotCom/ON_REFRESH_DATA',
  REFRESHED_DATA_RECEIVED = '@@cryptoDotCom/REFRESHED_DATA_RECEIVED',
  ON_TOTAL_PRICE_OPT_IN = '@@cryptoDotCom/ON_TOTAL_PRICE_OPT_IN',
  ON_BTC_PRICE_OPT_IN = '@@cryptoDotCom/ON_BTC_PRICE_OPT_IN',
  ON_BUY_CRYPTO = '@@cryptoDotCom/ON_BUY_CRYPTO',
  ON_INTERACTION = '@@cryptoDotCom/ON_INTERACTION',
  ON_MARKETS_OPT_IN = '@@cryptoDotCom/ON_MARKETS_OPT_IN',
}
