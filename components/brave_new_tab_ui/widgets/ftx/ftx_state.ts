// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { ChartDataPoint } from '../shared/chart'

export enum ViewType {
  Markets,
  Convert,
  Summary,
  AssetDetail,
  OptIn
}

export type AssetDetail = {
  currencyName: string
  marketData?: chrome.ftx.TokenPriceData
  chartData?: string | ChartDataPoint[] // string for error, undefined for not-fetched yet
}

export type FTXState = {
  hasInitialized: boolean
  isConnected: boolean
  balances: chrome.ftx.Balances
  balanceTotal: number | null
  marketData: chrome.ftx.TokenPriceData[]
  currencyNames: string[]
  currentView: ViewType
  assetDetail: null | AssetDetail
  conversionInProgress?: ConversionData
  ftxHost: chrome.ftx.FTXOauthHost
}

export type ConversionQuote = chrome.ftx.QuoteInfo & {
  quoteId: string
}

export type ConversionData = {
  from: string
  to: string
  quantity: number
  isSubmitting: boolean
  complete: boolean
  quote?: ConversionQuote // undefined means quote fetch in progress
}
