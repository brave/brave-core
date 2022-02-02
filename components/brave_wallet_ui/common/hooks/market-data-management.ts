import * as React from 'react'
import { CoinMarketMetadata, MarketDataTableColumnTypes, SortOrder } from '../../constants/types'

export const useMarketDataManagement = (marketData: CoinMarketMetadata[], sortOrder: SortOrder, columnId: MarketDataTableColumnTypes) => {
  const sortCoinMarketData = React.useCallback(() => {
    const sortedMarketData = [...marketData]

    if (sortOrder === 'asc') {
      sortedMarketData.sort((a, b) => a[columnId] - b[columnId])
    } else {
      sortedMarketData.sort((a, b) => b[columnId] - a[columnId])
    }

    return sortedMarketData
  }, [marketData, sortOrder, columnId])

  const filterCoinMarketData = React.useCallback((searchList: CoinMarketMetadata[], searchTerm: string) => {
    if (!searchTerm) {
      return searchList
    }

    const lowerCaseSearchTerm = searchTerm.toLowerCase()

    const filterCoinMarketData = searchList.filter(coin => {
      const { symbol, name } = coin

      return symbol.toLowerCase().includes(lowerCaseSearchTerm) ||
        name.toLowerCase().includes(lowerCaseSearchTerm)
    })
    console.log(filterCoinMarketData)

    return filterCoinMarketData
  }, [marketData])

  return {
    sortCoinMarketData,
    filterCoinMarketData
  }
}
