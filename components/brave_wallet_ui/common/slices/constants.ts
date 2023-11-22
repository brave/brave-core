// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { BraveWallet, CoinTypes } from '../../constants/types'

const s = 1000

export const maxBatchSizePrice = 25
export const maxConcurrentPriceRequests = 2
export const querySubscriptionOptions60s = {
  refetchOnFocus: true,
  pollingInterval: 60 * s,
  refetchOnMountOrArgChange: 60 * s,
  refetchOnReconnect: true
}
export const defaultQuerySubscriptionOptions = {
  refetchOnFocus: true,
  pollingInterval: 15 * s,
  refetchOnMountOrArgChange: 15 * s,
  refetchOnReconnect: true
}

export const coinTypesMapping = {
  [BraveWallet.CoinType.SOL]: CoinTypes.SOL,
  [BraveWallet.CoinType.ETH]: CoinTypes.ETH,
  [BraveWallet.CoinType.FIL]: CoinTypes.FIL,
  [BraveWallet.CoinType.BTC]: CoinTypes.BTC,
  [BraveWallet.CoinType.ZEC]: CoinTypes.ZEC
}
