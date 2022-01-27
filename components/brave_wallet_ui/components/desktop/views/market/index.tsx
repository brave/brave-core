import * as React from 'react'

import { SearchBar } from '../../../shared'
import { AssetsFilterDropdown } from '../..'
import { AssetFilterOptions } from '../../../../options/market-data-filter-options'

import {
  LoadIcon,
  LoadIconWrapper,
  StyledWrapper,
  TopRow
} from './style'
import { useMarketDataManagement } from '../../../../common/hooks/market-data-management'
import { marketDataTableHeaders } from '../../../../options/market-data-headers'
import { AssetFilterOption, BraveWallet, MarketDataTableColumnTypes, SortOrder } from '../../../../constants/types'
import MarketDataTable from '../../../../components/market-datatable'
import { debounce } from '../../../../../common/debounce'

export interface Props {
  isLoadingCoinMarketData: boolean
  onFetchCoinMarkets: (vsAsset: string, limit: number) => void
  coinMarkets: BraveWallet.CoinMarket[]
  tradableAssets: BraveWallet.BlockchainToken[]
}

const MarketView = (props: Props) => {
  const { isLoadingCoinMarketData, coinMarkets, tradableAssets, onFetchCoinMarkets } = props
  const [coinsMarketData, setCoinsMarketData] = React.useState<BraveWallet.CoinMarket[]>([])
  const [tableHeaders, setTableHeaders] = React.useState(marketDataTableHeaders)
  const [currentFilter, setCurrentFilter] = React.useState<AssetFilterOption>('all')
  const [sortOrder, setSortOrder] = React.useState<SortOrder>('desc')
  const [sortByColumnId, setSortByColumnId] = React.useState<MarketDataTableColumnTypes>('marketCap')
  const { sortCoinMarketData, searchCoinMarkets } = useMarketDataManagement(coinsMarketData, sortOrder, sortByColumnId)
  const [currentPage, setCurrentPage] = React.useState<number>(1)
  const [searchTerm, setSearchTerm] = React.useState('')
  const [moreDataAvailable, setMoreDataAvailable] = React.useState<boolean>(false)
  const defaultCurrency = 'usd'
  const assetsRequestLimit = 250
  const assetsPerPage = 20

  const search = (query: string) => {
    const filtered = filterCoinMarkets(coinMarkets, currentFilter)
    const searchResults = searchCoinMarkets(filtered, query)
    setCoinsMarketData(paginate(searchResults, assetsPerPage, 1))
    setCurrentPage(1)
  }

  const debounceSearch = (query: string) => {
    return debounce(search, 500)(query)
  }

  const onSelectFilter = (value: AssetFilterOption) => {
    setCurrentFilter(value)
  }

  const paginate = (coinMarkets: BraveWallet.CoinMarket[], perPage: number, pageNumber: number) => {
    const pageData = coinMarkets.slice((pageNumber - 1) * perPage, pageNumber * perPage)
    const nextPageData = coinMarkets.slice(pageNumber * perPage, (pageNumber + 1) * perPage)
    setMoreDataAvailable(nextPageData.length > 0)

    return pageData
  }

  const tradableAssetsSymbols = React.useMemo(() => {
    return tradableAssets.map(asset => asset.symbol.toLowerCase())
  }, [tradableAssets])

  const filterCoinMarkets = (coins: BraveWallet.CoinMarket[], filter: AssetFilterOption) => {
    if (filter === 'all') {
      return coins
    } else if (filter === 'tradable') {
      const filtered = coins.filter(asset => tradableAssetsSymbols.includes(asset.symbol.toLowerCase()))
      return filtered
    }

    return []
  }

  const getNextPage = () => {
    const nextPage = currentPage + 1
    const searchResults = searchCoinMarkets(coinMarkets, searchTerm)
    const filteredCoinMarkets = filterCoinMarkets(searchResults, currentFilter)
    const nextPageData = paginate(filteredCoinMarkets, assetsPerPage, nextPage)
    setCoinsMarketData([...visibleCoinMarkets, ...nextPageData])
    setCurrentPage(nextPage)
  }

  const onSort = (columnId: MarketDataTableColumnTypes, newSortOrder: SortOrder) => {
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
  }

  React.useEffect(() => {
    if (coinMarkets.length === 0) {
      onFetchCoinMarkets(defaultCurrency, assetsRequestLimit)
    }
  }, [])

  React.useEffect(() => {
    const filteredCoinMarkets = filterCoinMarkets(coinMarkets, currentFilter)
    setCoinsMarketData(paginate(filteredCoinMarkets, assetsPerPage, currentPage))
  }, [coinMarkets])

  React.useEffect(() => {
    const searchResults = searchCoinMarkets(coinMarkets, searchTerm)
    const filteredCoinMarkets = filterCoinMarkets(searchResults, currentFilter)
    setCoinsMarketData(paginate(filteredCoinMarkets, assetsPerPage, 1))
    setCurrentPage(1)
  }, [currentFilter])

  const visibleCoinMarkets = React.useMemo(() => {
    return sortCoinMarketData()
  }, [sortOrder, sortByColumnId, coinsMarketData])

  return (
    <StyledWrapper>
      <TopRow>
        <AssetsFilterDropdown
          options={AssetFilterOptions()}
          value={currentFilter}
          onSelectFilter={onSelectFilter}
        />
        <SearchBar
          placeholder="Search"
          autoFocus={true}
          action={event => {
            setSearchTerm(event.target.value)
            debounceSearch(event.target.value)
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
            moreDataAvailable={moreDataAvailable}
            showEmptyState={searchTerm !== '' || currentFilter !== 'all'}
            onFetchMoreMarketData={getNextPage}
            onSort={onSort}
          />
      }
    </StyledWrapper>
  )
}

export default MarketView
