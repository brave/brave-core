import * as React from 'react'
import InfinitieScroll from 'react-infinite-scroll-component'
import { getLocale } from '../../../common/locale'
import { BraveWallet, MarketDataTableColumnTypes, SortOrder } from '../../constants/types'
import Table, { Cell, Header, Row } from '../shared/datatable'
import {
  AssetsColumnItemSpacer,
  AssetsColumnWrapper,
  StyledWrapper,
  TableWrapper,
  TextWrapper
} from './style'
import {
  formatFiatAmountWithCommasAndDecimals,
  formatPricePercentageChange,
  formatPriceWithAbbreviation
} from '../../utils/format-prices'
import AssetNameAndIcon from '../asset-name-and-icon'
import AssetPriceChange from '../asset-price-change'
import { LoadIcon, LoadIconWrapper } from '../desktop/views/market/style'

export interface MarketDataHeader extends Header {
  id: MarketDataTableColumnTypes
}

export interface Props {
  headers: MarketDataHeader[]
  coinMarketData: BraveWallet.CoinMarket[]
  moreDataAvailable: boolean
  showEmptyState: boolean
  onFetchMoreMarketData: () => void
  onSort?: (column: MarketDataTableColumnTypes, newSortOrder: SortOrder) => void
  onSelectCoinMarket: (coinMarket: BraveWallet.CoinMarket) => void
}

const MarketDataTable = (props: Props) => {
  const { headers, coinMarketData, moreDataAvailable, showEmptyState, onFetchMoreMarketData, onSort, onSelectCoinMarket } = props

  const renderCells = (coinMarkDataItem: BraveWallet.CoinMarket) => {
    const {
      name,
      symbol,
      image,
      currentPrice,
      priceChange24h,
      priceChangePercentage24h,
      marketCap,
      marketCapRank,
      totalVolume
    } = coinMarkDataItem

    const formattedPrice = formatFiatAmountWithCommasAndDecimals(currentPrice.toString(), 'USD')
    const formattedPercentageChange = formatPricePercentageChange(priceChangePercentage24h, true)
    const formattedMarketCap = formatPriceWithAbbreviation(marketCap.toString(), 'USD', 1)
    const formattedVolume = formatPriceWithAbbreviation(totalVolume.toString(), 'USD', 1)
    const isDown = priceChange24h < 0

    const cellsContent: React.ReactNode[] = [
      <AssetsColumnWrapper>
        {/* Hidden until wishlist feature is available on the backend */}
        {/* <AssetsColumnItemSpacer>
          <AssetWishlistStar active={true} />
        </AssetsColumnItemSpacer> */}
        <AssetsColumnItemSpacer>
          <TextWrapper alignment="center">{marketCapRank}</TextWrapper>
        </AssetsColumnItemSpacer>
        <AssetNameAndIcon
          assetName={name}
          symbol={symbol}
          assetLogo={image}
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
      <TextWrapper alignment="right">{formattedVolume}</TextWrapper>

      // Line Chart Column
      // Commented out because priceHisotry data is yet to be
      // available from the backend
      // <LineChartWrapper>
      //   <LineChart
      //     priceData={priceHistory}
      //     isLoading={false}
      //     isDisabled={false}
      //     isDown={isDown}
      //     isAsset={true}
      //     onUpdateBalance={() => {}}
      //     showPulsatingDot={false}
      //     showTooltip={false}
      //     customStyle={{
      //       height: '20px',
      //       width: '100%',
      //       marginBottom: '0px'
      //     }}
      //   />
      // </LineChartWrapper>
    ]

    const cells: Cell[] = cellsContent.map(cellContent => {
      return {
        content: cellContent
      }
    })

    return cells
  }

  const rows: Row[] = React.useMemo(() => {
    const rows = coinMarketData.map((coinMarketItem: BraveWallet.CoinMarket) => {
      const cells = renderCells(coinMarketItem)
      const row = {
        content: cells,
        data: coinMarketItem,
        onClick: onSelectCoinMarket
      } as Row

      return row
    })

    return rows
  }, [coinMarketData, headers])

  return (
    <StyledWrapper>
        <InfinitieScroll
          dataLength={coinMarketData.length}
          next={onFetchMoreMarketData}
          loader={
            <LoadIconWrapper>
              <LoadIcon />
            </LoadIconWrapper>
          }
          hasMore={moreDataAvailable}
          style={{
            overflow: 'inherit'
          }}
        >
          <TableWrapper>
            <Table
              id='market-datatable'
              headers={headers}
              rows={rows}
              onSort={onSort}
              stickyHeaders={true}
            >
              {/* Empty state message */}
              {showEmptyState && getLocale('braveWalletMarketDataNoAssetsFound')}
            </Table>
          </TableWrapper>
        </InfinitieScroll>
    </StyledWrapper>
  )
}

export default MarketDataTable
