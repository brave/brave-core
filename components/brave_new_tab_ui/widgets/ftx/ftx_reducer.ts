// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { createReducer } from 'redux-act'
import * as Actions from './ftx_actions'
import { FTXState, ViewType, AssetDetail } from './ftx_state'

const defaultState: FTXState = {
  hasInitialized: false,
  isConnected: false,
  ftxHost: 'ftx.us',
  balances: {},
  balanceTotal: null,
  assetDetail: null,
  marketData: [],
  currencyNames: [],
  currentView: ViewType.OptIn
}

const reducer = createReducer<FTXState>({}, defaultState)

function convertBalanceToUSD (marketData: chrome.ftx.TokenPriceData[], currencyName: string, balance: number) {
  if (currencyName.toLowerCase() === 'usd') {
    return balance
  }
  let marketCurrencyMatch = marketData.find(m => m.symbol === currencyName)
  if (marketCurrencyMatch && marketCurrencyMatch.price) {
    const balanceInCurrency: number = balance * marketCurrencyMatch.price
    return balanceInCurrency
  } else {
    return null
  }
}

function stateWithBalancesAndMarkets (state: FTXState, incomingMarketData: chrome.ftx.TokenPriceData[], balances: chrome.ftx.Balances): FTXState {
  // Sum balances
  const marketData = incomingMarketData || state.marketData
  let balanceTotal = null
  if (marketData && balances) {
    for (const currency of Object.keys(balances)) {
      const currencyAmount = balances[currency]
      if (!currencyAmount) {
        continue
      }
      // Convert to same currency
      const balanceUSD = convertBalanceToUSD(marketData, currency, currencyAmount)
      if (balanceUSD === null) {
        // If there is a currency that we cannot find, don't display
        // any total, so that we aren't misleading.
        balanceTotal = null
        break
      }
      balanceTotal = (balanceTotal || 0) + balanceUSD
    }
  }

  // Extract currency name list
  if (incomingMarketData) {
    state = {
      ...state,
      currencyNames: incomingMarketData.map(i => i.symbol)
    }
  }
  return {
    ...state,
    balances,
    balanceTotal,
    marketData: incomingMarketData
  }
}

reducer.on(Actions.initialized, (state, payload) => {
  return {
    ...stateWithBalancesAndMarkets(state, payload.marketData, payload.balances),
    hasInitialized: true,
    isConnected: payload.isConnected,
    ftxHost: payload.ftxHost,
    // Initial view decision
    currentView: payload.isConnected ? ViewType.Markets : ViewType.OptIn
  }
})

reducer.on(Actions.disconnect, (state) => {
  return defaultState
})

reducer.on(Actions.openView, (state, viewDestination) => {
  // Change view and clear out any view-temporary data
  return {
    ...state,
    currentView: viewDestination,
    assetDetail: null,
    conversionInProgress: undefined
  }
})

reducer.on(Actions.preOptInViewMarkets, (state, payload) => {
  return {
    ...state,
    currentView: payload.hide ? ViewType.OptIn : ViewType.Markets,
    assetDetail: null,
    conversionInProgress: undefined
  }
})

reducer.on(Actions.dataUpdated, (state, payload) => {
  return stateWithBalancesAndMarkets(state, payload.marketData, payload.balances)
})

reducer.on(Actions.showAssetDetail, (state, payload) => {
  // Reset asset detail, normalize market data
  const marketData = state.marketData.find(d => d.symbol === payload.symbol)
  state = {
    ...state,
    assetDetail: {
      currencyName: payload.symbol,
      marketData
    }
  }
  return state
})

reducer.on(Actions.hideAssetDetail, (state) => {
  return {
    ...state,
    assetDetail: null
  }
})

reducer.on(Actions.assetChartDataUpdated, (state, payload) => {
  // Verify we haven't changed currency or closed detail screen
  // whilst waiting for http response.
  if (state.assetDetail?.currencyName !== payload.currencyName) {
    return state
  }
  const assetDetail: AssetDetail = {
    ...state.assetDetail
  }
  // Detect error
  if (!payload.chartData?.length) {
    assetDetail.chartData = 'Error'
  } else {
    assetDetail.chartData = payload.chartData
  }
  // Valid data
  return {
    ...state,
    assetDetail
  }
})

reducer.on(Actions.previewConversion, (state, payload) => {
  return {
    ...state,
    conversionInProgress: {
      ...payload,
      isSubmitting: false,
      complete: false
    }
  }
})

function closeOrCancelConversion (state: FTXState): FTXState {
  return {
    ...state,
    conversionInProgress: undefined
  }
}

reducer.on(Actions.cancelConversion, closeOrCancelConversion)

reducer.on(Actions.closeConversion, closeOrCancelConversion)

reducer.on(Actions.conversionQuoteAvailable, (state, payload) => {
  if (!state.conversionInProgress) {
    console.warn('FTX: conversion data came back when not in progress!')
    return state
  }
  return {
    ...state,
    conversionInProgress: {
      ...state.conversionInProgress,
      quote: payload
    }
  }
})

reducer.on(Actions.submitConversion, (state) => {
  // Validate
  if (!state.conversionInProgress) {
    console.warn('FTX: conversion cancelled when not in progress!')
    return state
  }
  return {
    ...state,
    conversionInProgress: {
      ...state.conversionInProgress,
      isSubmitting: true
    }
  }
})

reducer.on(Actions.conversionWasSuccessful, (state) => {
  // Validate
  if (!state.conversionInProgress) {
    console.warn('FTX: conversion successful when not in progress!')
    return state
  }
  return {
    ...state,
    conversionInProgress: {
      ...state.conversionInProgress,
      isSubmitting: false,
      complete: true
    }
  }
})

export default reducer
