// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import icons from './assets/icons'

// Returns 'Monday, July 20 2020' style.
function convertTimeToHumanReadable (time: string) {
  const d = new Date(time)
  const months = ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September', 'October', 'November', 'December']
  const days = ['Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday']
  return `${days[d.getDay()]}, ${months[d.getMonth()]} ${d.getDate()} ${d.getFullYear()}`
}

function renderIconAsset (key: string) {
  if (!(key in icons)) {
    return null
  }

  return (
    <>
      <img width={25} src={icons[key]} />
    </>
  )
}

function formattedNum (price: number) {
  return new Intl.NumberFormat('en-US', {
    style: 'currency',
    currency: 'USD',
    currencyDisplay: 'narrowSymbol'
  }).format(price)
}

function decimalizeCurrency (currencyAvailable: string, currencyDecimals: number) {
  return Number(Number(currencyAvailable).toFixed(currencyDecimals))
}

function getPercentColor (percentChange: string) {
  const percentChangeNum = parseFloat(percentChange)
  return percentChangeNum === 0 ? 'light' : (percentChangeNum > 0 ? 'green' : 'red')
}

// Merges losers/gainers into one table
function transformLosersGainers ({ losers = [], gainers = [] }: Record<string, chrome.cryptoDotCom.AssetRanking[]>): Record<string, chrome.cryptoDotCom.AssetRanking> {
  const losersGainersMerged = [ ...losers, ...gainers ]
  return losersGainersMerged.reduce((mergedTable: object, asset: chrome.cryptoDotCom.AssetRanking) => {
    let { pair: assetName } = asset
    assetName = assetName.split('_')[0]

    return {
      ...mergedTable,
      [assetName]: asset
    }
  }, {})
}

export {
  convertTimeToHumanReadable,
  decimalizeCurrency,
  formattedNum,
  getPercentColor,
  renderIconAsset,
  transformLosersGainers
}
