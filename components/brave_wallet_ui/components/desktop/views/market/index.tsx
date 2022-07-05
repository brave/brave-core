// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector, useDispatch } from 'react-redux'
import { useHistory } from 'react-router'

// Constants
import {
  AssetFilterOption,
  BraveWallet,
  MarketDataTableColumnTypes,
  SortOrder,
  WalletRoutes,
  WalletState
} from '../../../../constants/types'

// Actions
import { WalletActions } from '../../../../common/actions'

// Options
import { AssetFilterOptions } from '../../../../options/market-data-filter-options'
import { marketDataTableHeaders } from '../../../../options/market-data-headers'

// Components
import { SearchBar } from '../../../shared'
import { AssetsFilterDropdown } from '../..'
import { MarketDataTable } from '../../../market-datatable'

// Styled Components
import {
  LoadIcon,
  LoadIconWrapper,
  StyledWrapper,
  TopRow
} from './style'

// Hooks
import { useSwap } from '../../../../common/hooks'

// Utils
import { searchCoinMarkets, sortCoinMarkets, filterCoinMarkets } from '../../../../utils/coin-market-utils'
import { WalletPageActions } from '../../../../page/actions'

const defaultCurrency = 'usd'
const assetsRequestLimit = 250
const displayCount = 10

export const MarketView = () => {
  // State
  const [tableHeaders, setTableHeaders] = React.useState(marketDataTableHeaders)
  const [currentFilter, setCurrentFilter] = React.useState<AssetFilterOption>('all')
  const [sortOrder, setSortOrder] = React.useState<SortOrder>('desc')
  const [sortByColumnId, setSortByColumnId] = React.useState<MarketDataTableColumnTypes>('marketCap')
  const [searchTerm, setSearchTerm] = React.useState('')
  const [coinsDisplayCount, setCoinsDisplayCount] = React.useState(displayCount)

  // Redux
  const dispatch = useDispatch()
  const {
    isLoadingCoinMarketData,
    coinMarketData: allCoins
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  // Hooks
  const history = useHistory()

  // Custom Hooks
  const swap = useSwap()
  const { swapAssetOptions: tradableAssets } = swap

  // Memos
  const visibleCoinMarkets = React.useMemo(() => {
    const searchResults = searchTerm === '' ? allCoins : searchCoinMarkets(allCoins, searchTerm)
    const filteredCoins = filterCoinMarkets(searchResults, tradableAssets, currentFilter)
    const sorted = sortCoinMarkets(filteredCoins, sortOrder, sortByColumnId)
    return sorted.slice(0, coinsDisplayCount)
  }, [allCoins, sortOrder, sortByColumnId, coinsDisplayCount, searchTerm, currentFilter, tradableAssets])

  const onSelectFilter = (value: AssetFilterOption) => {
    setCurrentFilter(value)
  }

  const onSort = React.useCallback((columnId: MarketDataTableColumnTypes, newSortOrder: SortOrder) => {
    const updatedTableHeaders = tableHeaders.map(header => {
      if (header.id === columnId) {
        return {
          ...header,
          sortOrder: newSortOrder
        }
      } else {
        return {
          ...header,
          sortOrder: undefined
        }
      }
    })

    setTableHeaders(updatedTableHeaders)
    setSortByColumnId(columnId)
    setSortOrder(newSortOrder)
  }, [])

  const onShowMoreCoins = React.useCallback(() => {
    setCoinsDisplayCount(currentCount => currentCount + displayCount)
  }, [])

  const onSelectCoinMarket = (coinMarket: BraveWallet.CoinMarket) => {
    dispatch(WalletPageActions.selectCoinMarket(coinMarket))
    history.push(`${WalletRoutes.Market}/${coinMarket.symbol}`)
  }

  React.useEffect(() => {
    if (allCoins.length === 0) {
      dispatch(WalletActions.getCoinMarkets({
        vsAsset: defaultCurrency,
        limit: assetsRequestLimit
      }))
    }
  }, [allCoins])

  return (
    <StyledWrapper>
      <TopRow>
        <AssetsFilterDropdown
          options={AssetFilterOptions}
          value={currentFilter}
          onSelectFilter={onSelectFilter}
        />
        <SearchBar
          placeholder="Search"
          autoFocus={true}
          action={event => {
            setSearchTerm(event.target.value)
          }}
          disabled={isLoadingCoinMarketData}
        />
      </TopRow>
      {isLoadingCoinMarketData
        ? <LoadIconWrapper>
          <LoadIcon />
        </LoadIconWrapper>
        : <MarketDataTable
          headers={tableHeaders}
          coinMarketData={visibleCoinMarkets}
          moreDataAvailable={visibleCoinMarkets.length > 0}
          showEmptyState={searchTerm !== '' || currentFilter !== 'all'}
          onShowMoreCoins={onShowMoreCoins}
          onSort={onSort}
          onSelectCoinMarket={onSelectCoinMarket}
        />
      }
    </StyledWrapper>
  )
}
