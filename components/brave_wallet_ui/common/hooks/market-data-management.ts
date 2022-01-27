import * as React from 'react'
import Fuse from 'fuse.js'
import { BraveWallet, MarketDataTableColumnTypes, SortOrder } from '../../constants/types'

export const useMarketDataManagement = (marketData: BraveWallet.CoinMarket[], sortOrder: SortOrder, columnId: MarketDataTableColumnTypes) => {
  const sortCoinMarketData = React.useCallback(() => {
    const sortedMarketData = [...marketData]

    if (sortOrder === 'asc') {
      sortedMarketData.sort((a, b) => a[columnId] - b[columnId])
    } else {
      sortedMarketData.sort((a, b) => b[columnId] - a[columnId])
    }

    return sortedMarketData
  }, [marketData, sortOrder, columnId])

  const searchCoinMarkets = React.useCallback((searchList: BraveWallet.CoinMarket[], searchTerm: string) => {
    if (!searchTerm) {
      return searchList
    }

    const options = {
      shouldSort: true,
      threshold: 0.1,
      location: 0,
      distance: 0,
      minMatchCharLength: 1,
      keys: [
        { name: 'name', weight: 0.5 },
        { name: 'symbol', weight: 0.5 }
      ]
    }

    const fuse = new Fuse(searchList, options)
    const results = fuse.search(searchTerm).map((result: Fuse.FuseResult<BraveWallet.CoinMarket>) => result.item)

    return results
  }, [marketData])

  return {
    sortCoinMarketData,
    searchCoinMarkets
  }
}
