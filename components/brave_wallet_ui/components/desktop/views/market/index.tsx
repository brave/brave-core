import * as React from 'react'

import { SearchBar } from '../../../shared'
import { AssetsFilterDropdown, LineChart } from '../..'
import { AssetFilterOptions } from '../../../../options/market-data-filter-options'
import { CoinMarketMetadata, MarketDataTableColumnTypes, SortOrder } from '../../../../constants/types'
import MarketDataTable, { MarketDataHeader } from '../../../../components/market-datatable'
import { Cell, Row } from '../../../../components/shared/datatable'
import AssetWishlistStar from '../../../../components/asset-wishlist-star'
import AssetPriceChange from '../../../../components/asset-price-change'
import AssetNameAndIcon from '../../../../components/asset-name-and-icon'
import {
  formatFiatAmountWithCommasAndDecimals,
  formatPricePercentageChange,
  formatPriceWithAbbreviation
} from '../../../../utils/format-prices'

import {
  AssetsColumnWrapper,
  AssetsColumnItemSpacer,
  StyledWrapper,
  TopRow,
  TextWrapper,
  LineChartWrapper
} from './style'
import { useMarketDataManagement } from '../../../../common/hooks/market-data-management'

export interface Props {
  coinMarkData: CoinMarketMetadata[]
  marketDataTableHeaders: MarketDataHeader[]
}

const MarketView = (props: Props) => {
  const { coinMarkData, marketDataTableHeaders } = props
  const [sortedMarketData, setSortedMarketData] = React.useState(coinMarkData)
  const [tableHeaders, setTableHeaders] = React.useState(marketDataTableHeaders)
  const [currentFilter, setCurrentFilter] = React.useState('all')
  const [sortOrder, setSortOrder] = React.useState<SortOrder>('desc')
  const [sortByColumnId, setSortByColumnId] = React.useState<MarketDataTableColumnTypes>('marketCap')

  const { sortCoinMarketData, filterCoinMarketData } = useMarketDataManagement(sortedMarketData, sortOrder, sortByColumnId)

  const onSearch = (event: any) => {
    const searchTerm = event.target.value
    const filteredCoinMarketData = filterCoinMarketData(coinMarkData, searchTerm)
    setSortedMarketData(filteredCoinMarketData)
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
    const updatedMarketData = sortCoinMarketData()
    setSortedMarketData(updatedMarketData)
  }, [sortOrder, sortByColumnId])

  const renderCells = (coinMarkDataItem: CoinMarketMetadata) => {
    const {
      name,
      symbol,
      imageUrl,
      currentPrice,
      priceChange24h,
      priceChangePercentage24h,
      marketCap,
      marketCapRank,
      totalVolume,
      priceHistory
    } = coinMarkDataItem

    const formattedPrice = formatFiatAmountWithCommasAndDecimals(currentPrice.toString(), 'USD')
    const formattedPercentageChange = formatPricePercentageChange(priceChangePercentage24h, true)
    const formattedMarketCap = formatPriceWithAbbreviation(marketCap.toString(), 'USD', 1)
    const formattedVolume = formatPriceWithAbbreviation(totalVolume.toString(), 'USD', 1)
    const isDown = priceChange24h < 0

    const cellsContent: React.ReactNode[] = [
      // Assets Column
      <AssetsColumnWrapper>
        <AssetsColumnItemSpacer>
          <AssetWishlistStar active={true} />
        </AssetsColumnItemSpacer>
        <AssetsColumnItemSpacer>
          <TextWrapper alignment="right">{marketCapRank}</TextWrapper>
        </AssetsColumnItemSpacer>
        <AssetNameAndIcon
          assetName={name}
          symbol={symbol}
          assetLogo={imageUrl}
        />
      </AssetsColumnWrapper>,

      // Price Column
      <TextWrapper alignment="right">{formattedPrice}</TextWrapper>,

      // Price Change Column
      <TextWrapper alignment="right">
        <AssetPriceChange
          isDown={isDown}
          priceChangePercentage={formattedPercentageChange}
        />
      </TextWrapper>,

      // Market Cap Column
      <TextWrapper alignment="right">{formattedMarketCap}</TextWrapper>,

      // Volume Column
      <TextWrapper alignment="right">{formattedVolume}</TextWrapper>,

      // Line Chart Column
      <LineChartWrapper>
        <LineChart
          priceData={priceHistory}
          isLoading={false}
          isDisabled={false}
          isDown={isDown}
          isAsset={true}
          onUpdateBalance={() => {}}
          showPulsatingDot={false}
          customStyle={{
            height: '20px',
            width: '100%',
            marginBottom: '0px'
          }}
        />
      </LineChartWrapper>
    ]

    const cells: Cell[] = cellsContent.map(cellContent => {
      return {
        content: cellContent
      }
    })

    return cells
  }

  const rows: Row[] = React.useMemo(() => {
    const cells = sortedMarketData.map((coinMarketItem: CoinMarketMetadata) => {
      return renderCells(coinMarketItem)
    })

    return cells.map(cell => { return { content: cell } })
  }, [sortedMarketData, marketDataTableHeaders])

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
        rows={rows}
        onSort={onSort}
      />
    </StyledWrapper>
  )
}

export default MarketView
