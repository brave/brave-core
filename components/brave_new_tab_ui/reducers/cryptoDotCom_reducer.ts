/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'
import { types } from '../constants/cryptoDotCom_types'

function reducePairs (rawPairs: chrome.cryptoDotCom.SupportedPair[]) {
  if (!rawPairs || !rawPairs.length) {
    return {}
  }
  return rawPairs.reduce((pairs: object, currPair: chrome.cryptoDotCom.SupportedPair) => {
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
      state = {
        ...state,
        cryptoDotComState: {
          ...state.cryptoDotComState,
          optInBTCPrice: true
        }
      }
      break

    case types.ON_IS_CONNECTED_RECEIVED:
      const isConnected = payload.isConnected
      state = {
        ...state,
        cryptoDotComState: {
          ...state.cryptoDotComState,
          isConnected: isConnected,
          // Reset account specific state if not connected.
          newsEvents: isConnected ? state.cryptoDotComState.newsEvents : [],
          depositAddresses: isConnected ? state.cryptoDotComState.depositAddresses : {},
          accountBalances: isConnected ? state.cryptoDotComState.accountBalances
                                       : { total_balance: '0', accounts: [] }
        }
      }
      break

    case types.SET_DISCONNECT_IN_PROGRESS:
      state = {
        ...state,
        cryptoDotComState: {
          ...state.cryptoDotComState,
          disconnectInProgress: payload.inProgress
        }
      }
      break

    case types.ALL_ASSETS_DETAILS_RECEIVED:
      state = {
        ...state,
        cryptoDotComState: {
          ...state.cryptoDotComState,
          charts: {
            ...state.cryptoDotComState.charts,
            ...payload.charts
          },
          tickerPrices: {
            ...state.cryptoDotComState.tickerPrices,
            ...payload.tickerPrices
          },
          depositAddresses: {
            ...state.cryptoDotComState.depositAddresses,
            [payload.depositAddress.currency]: {
              address: payload.depositAddress.address,
              qr_code: payload.depositAddress.qr_code
            }
          }
        }
      }
      break

    case types.HIDE_BALANCE:
      state = {
        ...state,
        cryptoDotComState: {
          ...state.cryptoDotComState,
          hideBalance: payload.hide
        }
      }
      break

    case types.REFRESHED_DATA_RECEIVED:
      state = {
        ...state,
        cryptoDotComState: {
          ...state.cryptoDotComState,
          tickerPrices: {
            ...state.cryptoDotComState.tickerPrices,
            ...payload.tickerPrices
          },
          losersGainers: payload.losersGainers,
          charts: payload.charts ? {
            ...state.cryptoDotComState.charts,
            ...payload.charts
          } : state.cryptoDotComState.charts,
          accountBalances: payload.accountBalances ? payload.accountBalances : state.cryptoDotComState.accountBalances,
          newsEvents: payload.newsEvents ? payload.newsEvents : state.cryptoDotComState.newsEvents,
          supportedPairs: payload.pairs ? reducePairs(payload.pairs) : state.cryptoDotComState.supportedPairs,
          tradingPairs: payload.pairs ? payload.pairs : state.cryptoDotComState.tradingPairs
        }
      }
      break

    default:
      break
  }

  return state
}

export default cryptoDotComReducer
