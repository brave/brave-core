/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'
import { types } from '../constants/cryptoDotCom_types'
import {
  onCryptoDotComMarketDataReceived,
  onCryptoDotComAssetsDetailsReceived,
  onCryptoDotComRefreshData
} from '../actions/cryptoDotCom_actions'
import {
  fetchCryptoDotComTickerPrices,
  fetchCryptoDotComLosersGainers,
  fetchCryptoDotComCharts,
  fetchCryptoDotComSupportedPairs
} from '../api/cryptoDotCom'

interface SupportedPair {
  base: string
  pair: string
  quote: string
}

function performSideEffect (fn: () => void): void {
  window.setTimeout(() => fn(), 0)
}

function reducePairs (rawPairs: SupportedPair[]) {
  if (!rawPairs || rawPairs.length <= 0) {
    return null
  }
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
      state.cryptoDotComState = {
        ...state.cryptoDotComState,
        optInBTCPrice: true
      }
      performSideEffect(async function () {
        chrome.cryptoDotCom.onInteraction()

        const [tickerPrices, losersGainers] = await Promise.all([
          fetchCryptoDotComTickerPrices(['BTC']),
          fetchCryptoDotComLosersGainers()
        ])
        onCryptoDotComMarketDataReceived(tickerPrices, losersGainers)
      })
      break

    case types.MARKETS_REQUESTED:
      state.cryptoDotComState = {
        ...state.cryptoDotComState,
        // fetchStatus: 'pending',
        optInMarkets: true
      }
      performSideEffect(async function () {
        chrome.cryptoDotCom.onInteraction()

        const [tickerPrices, losersGainers] = await Promise.all([
          fetchCryptoDotComTickerPrices(payload.markets),
          fetchCryptoDotComLosersGainers()
        ])
        onCryptoDotComMarketDataReceived(tickerPrices, losersGainers)
      })
      break

    case types.MARKETS_RECEIVED:
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

    case types.ALL_ASSETS_DETAILS_REQUESTED:
      state.cryptoDotComState = {
        ...state.cryptoDotComState,
        // fetchStatus: 'pending'
      }
      performSideEffect(async function () {
        const [charts, pairs] = await Promise.all([
          fetchCryptoDotComCharts(payload.assets),
          fetchCryptoDotComSupportedPairs()
        ])
        onCryptoDotComAssetsDetailsReceived(charts, pairs)
      })
      break

    case types.ALL_ASSETS_DETAILS_RECEIVED:
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
        // fetchStatus: 'refreshing'
      }
      performSideEffect(async function () {
        const { supportedPairs, tickerPrices: prices } = state.cryptoDotComState
        const assets = Object.keys(prices)
        const supportedPairsSet = Object.keys(supportedPairs).length

        const requests = [
          fetchCryptoDotComTickerPrices(assets),
          fetchCryptoDotComLosersGainers(),
          fetchCryptoDotComCharts(assets)
        ]

        // These are rarely updated, so we only need to fetch them
        // in the refresh interval if they aren't set yet (perhaps due to no connection)
        if (!supportedPairsSet) {
          requests.push(fetchCryptoDotComSupportedPairs())
        }

        const [tickerPrices, losersGainers, charts, newSupportedPairs] = await Promise.all(requests)

        onCryptoDotComRefreshData(tickerPrices, losersGainers, charts, newSupportedPairs)
      })
      break

    case types.REFRESHED_DATA_RECEIVED:
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
        losersGainers: payload.losersGainers,
        supportedPairs: reducePairs(payload.pairs) || state.cryptoDotComState.supportedPairs
      }
      break

    case types.ON_BUY_CRYPTO:
      performSideEffect(async function () {
        chrome.cryptoDotCom.onBuyCrypto()
      })
      break

    case types.ON_MARKETS_OPT_IN:
      state = { ...state }
      state.cryptoDotComState.optInMarkets = payload.show
      break

    default:
      break
  }

  return state
}

export default cryptoDotComReducer
