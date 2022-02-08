import * as React from 'react'
import InfinitieScroll from 'react-infinite-scroll-component'
import { CoinMarketMetadata, MarketDataTableColumnTypes, SortOrder } from '../../constants/types'
import Table, { Cell, Header, Row } from '../shared/datatable'
import {
  AssetsColumnWrapper,
  AssetsColumnItemSpacer,
  StyledWrapper,
  TableWrapper,
  TextWrapper,
  LineChartWrapper
} from './style'
import LineChart from '../desktop/line-chart'
import {
  formatFiatAmountWithCommasAndDecimals,
  formatPricePercentageChange,
  formatPriceWithAbbreviation
} from '../../utils/format-prices'
import AssetNameAndIcon from '../asset-name-and-icon'
import AssetPriceChange from '../asset-price-change'
import AssetWishlistStar from '../asset-wishlist-star'

export interface MarketDataHeader extends Header {
  id: MarketDataTableColumnTypes
}

export interface Props {
  headers: MarketDataHeader[]
  coinMarketData: CoinMarketMetadata[]
  onFetchMoreMarketData: () => void
  onSort?: (column: MarketDataTableColumnTypes, newSortOrder: SortOrder) => void
}

const MarketDataTable = (props: Props) => {
  const { headers, coinMarketData, onSort, onFetchMoreMarketData } = props

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
          showTooltip={false}
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
    const cells = coinMarketData.map((coinMarketItem: CoinMarketMetadata) => {
      return renderCells(coinMarketItem)
    })

    return cells.map(cell => { return { content: cell } })
  }, [coinMarketData, headers])

  return (
    <StyledWrapper>
      <InfinitieScroll
        dataLength={coinMarketData.length}
        next={onFetchMoreMarketData}
        loader={<div>Loading...</div>}
        endMessage={<div>You have seen it all</div>}
        hasMore={true}
        style={{ width: '100%' }}
      >
        <TableWrapper>
          <Table
            headers={headers}
            rows={rows}
            onSort={onSort}
          />
        </TableWrapper>
      </InfinitieScroll>
    </StyledWrapper>
  )
}

export default MarketDataTable
