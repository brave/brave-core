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
import { MarketDataTableHeaders } from '../../../../options/market-data-headers'
import { BraveWallet, MarketDataTableColumnTypes, SortOrder } from '../../../../constants/types'
import MarketDataTable from '../../../../components/market-datatable'

export interface Props {
  isLoadingCoinMarketData: boolean
  onFetchCoinMarkets: (vsAsset: string, limit: number) => void
  coinMarkets: BraveWallet.CoinMarket[]
}

const MarketView = (props: Props) => {
  const { isLoadingCoinMarketData, coinMarkets, onFetchCoinMarkets } = props
  const [coinsMarketData, setCoinsMarketData] = React.useState<BraveWallet.CoinMarket[]>([])
  const [tableHeaders, setTableHeaders] = React.useState(MarketDataTableHeaders)
  const [currentFilter, setCurrentFilter] = React.useState('all')
  const [sortOrder, setSortOrder] = React.useState<SortOrder>('desc')
  const [sortByColumnId, setSortByColumnId] = React.useState<MarketDataTableColumnTypes>('marketCap')
  const { sortCoinMarketData, filterCoinMarketData } = useMarketDataManagement(coinsMarketData, sortOrder, sortByColumnId)
  const [currentPage, setCurrentPage] = React.useState<number>(1)
  const [moreDataAvailable, setMoreDataAvailble] = React.useState<boolean>(true)
  const defaultCurrency = 'usd'
  const limit = 250
  const perPage = 30

  const onSearch = (event: any) => {
    const searchTerm = event.target.value
    if (searchTerm !== '') {
      const filteredCoinMarketData = filterCoinMarketData(coinsMarketData, searchTerm)
      setCoinsMarketData(filteredCoinMarketData)
    } else {
      setCoinsMarketData(sortedMarketData)
    }
  }

  const onSelectFilter = (value: string) => {
    setCurrentFilter(value)
  }

  const paginate = (coinMarkets: BraveWallet.CoinMarket[], perPage: number, pageNumber: number) => {
    return coinMarkets.slice((pageNumber - 1) * perPage, pageNumber * perPage)
  }

  const getNextPage = () => {
    const nextPage = currentPage + 1
    const data = paginate(coinMarkets, perPage, nextPage)
    if (data.length === 0) {
      setMoreDataAvailble(false)
    } else {
      setCoinsMarketData([...sortedMarketData, ...data])
    }
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
    onFetchCoinMarkets(defaultCurrency, limit)
  }, [])

  React.useEffect(() => {
    const currenPageData = paginate(coinMarkets, perPage, currentPage)
    setCoinsMarketData(currenPageData)
  }, [coinMarkets])

  const sortedMarketData = React.useMemo(() => {
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
          action={onSearch}
          disabled={isLoadingCoinMarketData}
        />
      </TopRow>
      {isLoadingCoinMarketData
        ? <LoadIconWrapper>
            <LoadIcon />
          </LoadIconWrapper>
        : <MarketDataTable
            headers={tableHeaders}
            coinMarketData={sortedMarketData}
            moreDataAvailable={moreDataAvailable}
            onFetchMoreMarketData={getNextPage}
            onSort={onSort}
          />
      }
    </StyledWrapper>
  )
}

export default MarketView
