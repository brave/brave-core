/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'
import { types } from '../constants/cryptoDotCom_types'

interface SupportedPair {
  base: string
  pair: string
  quote: string
}

function performSideEffect (fn: () => void): void {
  window.setTimeout(() => fn(), 0)
}

function reducePairs (rawPairs: SupportedPair[]) {
  return rawPairs.reduce((pairs: object, currPair: SupportedPair) => {
    const { base, pair } = currPair
    pairs[base] = pairs[base]
      ? [...pairs[base], pair]
      : [pair]
    return pairs
  }, {})
}

const cryptoDotComReducer: Reducer<NewTab.State | undefined> = (state: NewTab.State, action) => {
  const payload = action.payload

  switch (action.type) {
    case types.ON_BTC_PRICE_OPT_IN:
      state = { ...state }
      state.cryptoDotComState.optInBTCPrice = true
      break

    case types.MARKET_DATA_UPDATED:
      state = { ...state }
      state.cryptoDotComState = {
        ...state.cryptoDotComState,
        tickerPrices: {
          ...state.cryptoDotComState.tickerPrices,
          ...payload.tickerPrices
        },
        losersGainers: payload.losersGainers
      }
      break

    case types.SET_ASSET_DATA:
      state = { ...state }
      state.cryptoDotComState = {
        ...state.cryptoDotComState,
        charts: {
          ...state.cryptoDotComState.charts,
          ...payload.charts
        },
        supportedPairs: reducePairs(payload.pairs)
      }
      break

    case types.ON_REFRESH_DATA:
      state = { ...state }
      state.cryptoDotComState = {
        ...state.cryptoDotComState,
        tickerPrices: {
          ...state.cryptoDotComState.tickerPrices,
          ...payload.tickerPrices
        },
        charts: {
          ...state.cryptoDotComState.charts,
          ...payload.charts
        },
        losersGainers: payload.losersGainers
      }
      break

    case types.ON_BUY_CRYPTO:
      performSideEffect(async function () {
        chrome.cryptoDotCom.onBuyCrypto()
      })
      break

    case types.ON_INTERACTION:
      performSideEffect(async function () {
        chrome.cryptoDotCom.onInteraction()
      })
      break

    case types.ON_MARKETS_OPT_IN:
      state = { ...state }
      state.cryptoDotComState.optInMarkets = payload.show
      break

    case types.SET_SUPPORTED_PAIRS:
      state = { ...state }
      state.cryptoDotComState = {
        ...state.cryptoDotComState,
        supportedPairs: reducePairs(payload.pairs)
      }
      break

    default:
      break
  }

  return state
}

export default cryptoDotComReducer
