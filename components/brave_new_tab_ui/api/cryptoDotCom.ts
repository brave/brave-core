// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

function getCryptoDotComTickerInfo (asset: string) {
  return new Promise((resolve: Function) => {
    chrome.cryptoDotCom.getTickerInfo(asset, (resp: chrome.cryptoDotCom.TickerPrice) => {
      resolve({ [asset]: resp })
    })
  })
}

function getCryptoDotComAssetRankings () {
  return new Promise((resolve: Function) => {
    chrome.cryptoDotCom.getAssetRankings((resp: Record<string, chrome.cryptoDotCom.AssetRanking[]>) => {
      resolve(resp)
    })
  })
}

function getCryptoDotComChartData (asset: string) {
  return new Promise((resolve: Function) => {
    chrome.cryptoDotCom.getChartData(asset, (resp: chrome.cryptoDotCom.ChartDataPoint[]) => {
      resolve({ [asset]: resp })
    })
  })
}

function getCryptoDotComSupportedPairs () {
  return new Promise((resolve: Function) => {
    chrome.cryptoDotCom.getSupportedPairs((resp: chrome.cryptoDotCom.SupportedPair[]) => {
      resolve(resp)
    })
  })
}

function getCryptoDotComAccountBalances () {
  return new Promise((resolve: Function) => {
    chrome.cryptoDotCom.getAccountBalances((balance: chrome.cryptoDotCom.AccountBalances) => {
      resolve(balance)
    })
  })
}

function getCryptoDotComNewsEvents () {
  return new Promise((resolve: Function) => {
    chrome.cryptoDotCom.getNewsEvents((newsEvents: chrome.cryptoDotCom.NewsEvent[]) => {
      resolve(newsEvents)
    })
  })
}

function getCryptoDotComDepositAddress (asset: string) {
  return new Promise((resolve: Function) => {
    chrome.cryptoDotCom.getDepositAddress(asset, (address: chrome.cryptoDotCom.DepositAddress) => {
      resolve(address)
    })
  })
}

export async function fetchCryptoDotComTickerPrices (assets: string[]) {
  const assetReqs = assets.map(asset => getCryptoDotComTickerInfo(asset))
  const assetResps = await Promise.all(assetReqs).then((resps: object[]) => resps)
  return assetResps.reduce((all, current) => ({ ...current, ...all }), {})
}

export async function fetchCryptoDotComLosersGainers () {
  return getCryptoDotComAssetRankings().then((resp: Record<string, chrome.cryptoDotCom.AssetRanking[]>) => resp)
}

export async function fetchCryptoDotComCharts (assets: string[]) {
  const chartReqs = assets.map(asset => getCryptoDotComChartData(asset))
  const chartResps = await Promise.all(chartReqs).then((resps: object[]) => resps)
  return chartResps.reduce((all, current) => ({ ...current, ...all }), {})
}

export async function fetchCryptoDotComSupportedPairs () {
  return getCryptoDotComSupportedPairs().then((resp: chrome.cryptoDotCom.SupportedPair[]) => resp)
}

export async function fetchCryptoDotComAccountBalances () {
  return getCryptoDotComAccountBalances().then((balance: chrome.cryptoDotCom.AccountBalances) => balance)
}

export async function fetchCryptoDotComDepositAddress (asset: string) {
  return getCryptoDotComDepositAddress(asset).then((address: chrome.cryptoDotCom.DepositAddress) => address)
}

export async function fetchCryptoDotComNewsEvents () {
  return getCryptoDotComNewsEvents().then((newsEvents: chrome.cryptoDotCom.NewsEvent[]) => newsEvents)
}
