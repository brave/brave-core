// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '$web-common/locale'
import { BraveWallet, MarketDataTableColumnTypes, SortOrder } from '../../constants/types'
import Amount from '../../utils/amount'

// Styled components
import {
  AssetsColumnItemSpacer,
  AssetsColumnWrapper,
  StyledWrapper,
  TableWrapper,
  TextWrapper
} from './style'

// Components
import { Table, Cell, Header, Row } from '../shared/datatable'
import { AssetNameAndIcon } from '../asset-name-and-icon'
import { AssetPriceChange } from '../asset-price-change'
import { CoinGeckoText } from '../desktop/views/portfolio/style'

export interface MarketDataHeader extends Header {
  id: MarketDataTableColumnTypes
}

export interface Props {
  headers: MarketDataHeader[]
  coinMarketData: BraveWallet.CoinMarket[]
  showEmptyState: boolean
  onSort?: (column: MarketDataTableColumnTypes, newSortOrder: SortOrder) => void
  onSelectCoinMarket: (coinMarket: BraveWallet.CoinMarket) => void
}

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

  const formattedPrice = new Amount(currentPrice).formatAsFiat('USD')
  const formattedPercentageChange = new Amount(priceChangePercentage24h).value?.absoluteValue().toFixed(2) + '%'
  const formattedMarketCap = new Amount(marketCap).abbreviate(1, 'USD', 'billion')
  const formattedVolume = new Amount(totalVolume).abbreviate(1, 'USD')
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
    // Commented out because priceHistory data is yet to be
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

export const MarketDataTable = (props: Props) => {
  const {
    headers,
    coinMarketData,
    showEmptyState,
    onSort,
    onSelectCoinMarket
  } = props

  const rows: Row[] = React.useMemo(() => {
    return coinMarketData.map((coinMarketItem: BraveWallet.CoinMarket) => {
      return {
        id: `coin-row-${coinMarketItem.symbol}-${coinMarketItem.marketCapRank}`,
        content: renderCells(coinMarketItem),
        data: coinMarketItem,
        onClick: onSelectCoinMarket
      }
    })
  }, [coinMarketData])

  return (
    <StyledWrapper>
      <TableWrapper>
        <Table
          headers={headers}
          rows={rows}
          onSort={onSort}
          stickyHeaders={true}
        >
          {/* Empty state message */}
          {showEmptyState && getLocale('braveWalletMarketDataNoAssetsFound')}
        </Table>
      </TableWrapper>
      <CoinGeckoText>{getLocale('braveWalletPoweredByCoinGecko')}</CoinGeckoText>
    </StyledWrapper>
  )
}
