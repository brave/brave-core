// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { loadTimeData } from '../../common/loadTimeData'
import { BraveWallet } from '../constants/types'

const marketUiOrigin = loadTimeData.getString('braveWalletMarketUiBridgeUrl')

// remove trailing /
export const braveMarketUiOrigin = marketUiOrigin.endsWith('/') ? marketUiOrigin.slice(0, -1) : marketUiOrigin
export const braveWalletOrigin = 'chrome://wallet'

export const enum MarketUiCommand {
  UpdateCoinMarkets = 'update-coin-markets',
  SelectCoinMarket = 'select-coin-market',
  UpdateTradableAssets = 'update-tradable-assets'
}

export type MarketCommandMessage = {
  command: MarketUiCommand
}

export type UpdateCoinMarketMessage = MarketCommandMessage & {
  payload: BraveWallet.CoinMarket[]
}

export type SelectCoinMarketMessage = MarketCommandMessage & {
  payload: BraveWallet.CoinMarket
}

export type UpdateTradableAssetsMessage = MarketCommandMessage & {
  payload: BraveWallet.BlockchainToken[]
}

export const sendMessageToMarketUiFrame = (targetWindow: Window | null, message: MarketCommandMessage) => {
  if (targetWindow) {
    targetWindow.postMessage(message, braveMarketUiOrigin)
  }
}

export const sendMessageToWalletUi = (targetWindow: Window | null, message: MarketCommandMessage) => {
  if (targetWindow) {
    targetWindow.postMessage(message, braveWalletOrigin)
  }
}
