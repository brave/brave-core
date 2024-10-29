// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {
  BraveWallet,
  MarketAssetFilterOption,
  MarketGridColumnTypes,
  MeldCryptoCurrency,
  SortOrder
} from '../constants/types'
import { getAssetSymbol } from './meld_utils'

export const sortCoinMarkets = (
  marketData: BraveWallet.CoinMarket[],
  sortOrder: SortOrder,
  columnId: MarketGridColumnTypes
) => {
  if (sortOrder === 'asc') {
    return marketData.sort((a, b) => a[columnId] - b[columnId])
  } else {
    return marketData.sort((a, b) => b[columnId] - a[columnId])
  }
}

export const searchCoinMarkets = (
  searchList: BraveWallet.CoinMarket[],
  searchTerm: string
): BraveWallet.CoinMarket[] => {
  const trimmedSearch = searchTerm.trim().toLowerCase()
  if (!trimmedSearch) {
    return searchList
  }

  return searchList.filter(
    (coin) =>
      coin.name.toLowerCase().includes(trimmedSearch) ||
      coin.symbol.toLowerCase().includes(trimmedSearch)
  )
}

export const filterCoinMarkets = (
  coins: BraveWallet.CoinMarket[],
  tradableAssets: MeldCryptoCurrency[] | undefined,
  filter: MarketAssetFilterOption
) => {
  const tradableAssetsSymbols = tradableAssets?.map((asset) =>
    getAssetSymbol(asset).toLowerCase()
  )

  if (filter === 'all') {
    return coins
  } else if (filter === 'tradable') {
    return coins.filter((asset) =>
      tradableAssetsSymbols?.includes(asset.symbol.toLowerCase())
    )
  }

  return []
}
