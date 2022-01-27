import * as React from 'react'

import { SearchBar } from '../../../shared'
import { AssetsFilterDropdown } from '../..'
import { AssetFilterOptions } from '../../../../options/market-data-filter-options'

import {
  StyledWrapper,
  TopRow
} from './style'
import { useMarketDataManagement } from '../../../../common/hooks/market-data-management'
import { MarketDataTableHeaders } from '../../../../options/market-data-headers'
import { fetchCoinMarketData } from '../../../../stories/mock-data/mock-coin-market-data'

export interface Props {
  // coinsMarketData: CoinMarketMetadata[]
  // marketDataTableHeaders: MarketDataHeader[]
  // onFetchMoreMarketData: () => void
}

const MarketView = (props: Props) => {
  const [coinsMarketData, setCoinsMarketData] = React.useState<CoinMarketMetadata[]>([])
  const [tableHeaders, setTableHeaders] = React.useState(MarketDataTableHeaders)
  const [currentFilter, setCurrentFilter] = React.useState('all')
  const [sortOrder, setSortOrder] = React.useState<SortOrder>('desc')
  const [sortByColumnId, setSortByColumnId] = React.useState<MarketDataTableColumnTypes>('marketCap')
  const { sortCoinMarketData, filterCoinMarketData } = useMarketDataManagement(coinsMarketData, sortOrder, sortByColumnId)
  const [currentMarketDataPage, setCurrentMarketDataPage] = React.useState<number>(1)
  const [moreDataAvailable, setMoreDataAvailble] = React.useState<boolean>(true)
  const marketDataPageSize = 20

  const onSearch = (event: any) => {
    const searchTerm = event.target.value
    const filteredCoinMarketData = filterCoinMarketData(coinsMarketData, searchTerm)
    setCoinsMarketData(filteredCoinMarketData)
  }
  
  const onSelectFilter = (value: string) => {
    setCurrentFilter(value)
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
    // eslint-disable-next-line @typescript-eslint/no-unused-vars
    const fetchCoinsData = async () => {
      const coins = await fetchCoinMarketData(marketDataPageSize, currentMarketDataPage)
      setCoinsMarketData(coins)
      setCurrentMarketDataPage(currentMarketDataPage + 1)
    }

    fetchCoinsData()
  }, [])

  const sortedMarketData = React.useMemo(() => {
    return sortCoinMarketData()
  }, [sortOrder, sortByColumnId, coinsMarketData])

  const onFetchMoreMarketData = async () => {
    const coins = await fetchCoinMarketData(marketDataPageSize, currentMarketDataPage)
    if (coins.length === 0) {
      setMoreDataAvailble(false)
    } else {
      console.log(coins, 'loaded new coins')
      setCoinsMarketData([...coinsMarketData, ...coins])
      setCurrentMarketDataPage(currentMarketDataPage + 1)
    }
  }

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
        />
      </TopRow>
      <MarketDataTable
        headers={tableHeaders}
        coinMarketData={sortedMarketData}
        moreDataAvailable={moreDataAvailable}
        onFetchMoreMarketData={onFetchMoreMarketData}
        onSort={onSort}
      />
    </StyledWrapper>
  )
}

export default MarketView
