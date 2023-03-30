// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '$web-common/locale'
import {
  AccountButtonOptionsObjectType,
  BraveWallet,
  MarketDataTableColumnTypes,
  SortOrder
} from '../../constants/types'
import Amount from '../../utils/amount'

// Options
import { BuyButtonOption, DepositButtonOption } from '../../options/account-list-button-options'

// Styled components
import {
  AssetsColumnWrapper,
  StyledWrapper,
  TableWrapper,
  TextWrapper,
  ButtonsRow
} from './style'

// Components
import { Table, Cell, Header, Row } from '../shared/datatable'
import { AssetNameAndIcon } from '../asset-name-and-icon'
import { AssetPriceChange } from '../asset-price-change'
import { CoinGeckoText } from '../desktop/views/portfolio/style'
import { AccountListItemOptionButton } from '../desktop/account-list-item/account-list-item-option-button'

export interface MarketDataHeader extends Header {
  id: MarketDataTableColumnTypes
}

export interface Props {
  headers: MarketDataHeader[]
  coinMarketData: BraveWallet.CoinMarket[]
  showEmptyState: boolean
  onSort?: (column: MarketDataTableColumnTypes, newSortOrder: SortOrder) => void
  onSelectCoinMarket: (coinMarket: BraveWallet.CoinMarket) => void
  isBuySupported: (coinMarket: BraveWallet.CoinMarket) => boolean
  isDepositSupported: (coinMarket: BraveWallet.CoinMarket) => boolean
  onClickBuy: (coinMarket: BraveWallet.CoinMarket) => void
  onClickDeposit: (coinMarket: BraveWallet.CoinMarket) => void
  fiatCurrency: string
}

const renderCells = (
  coinMarkDataItem: BraveWallet.CoinMarket,
  buttonOptions: AccountButtonOptionsObjectType[],
  fiatCurrency: string,
  onClickBuy: (coinMarket: BraveWallet.CoinMarket) => void,
  onClickDeposit: (coinMarket: BraveWallet.CoinMarket) => void
) => {
  const {
    name,
    symbol,
    image,
    currentPrice,
    priceChange24h,
    priceChangePercentage24h,
    marketCap,
    totalVolume
  } = coinMarkDataItem

  const formattedPrice = new Amount(currentPrice).formatAsFiat(fiatCurrency)
  const formattedPercentageChange = new Amount(priceChangePercentage24h).value?.absoluteValue().toFixed(2) + '%'
  const formattedMarketCap = new Amount(marketCap).abbreviate(1, fiatCurrency, 'billion')
  const formattedVolume = new Amount(totalVolume).abbreviate(1, fiatCurrency)
  const isDown = priceChange24h < 0

  const cellsContent: React.ReactNode[] = [
    <AssetsColumnWrapper>
      {/* Hidden until wishlist feature is available on the backend */}
      {/* <AssetsColumnItemSpacer>
          <AssetWishlistStar active={true} />
        </AssetsColumnItemSpacer> */}
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
    <TextWrapper alignment="right">{formattedVolume}</TextWrapper>,

    <ButtonsRow>
      {buttonOptions.map((option) =>
        <AccountListItemOptionButton
          key={option.id}
          onClick={option.id === 'buy'
            ? () => onClickBuy(coinMarkDataItem)
            : () => onClickDeposit(coinMarkDataItem)}
          option={option}
          hideIcon={true}
        />
      )}
    </ButtonsRow>

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
    fiatCurrency,
    onSort,
    onSelectCoinMarket,
    isBuySupported,
    isDepositSupported,
    onClickBuy,
    onClickDeposit
  } = props

  // Memos
  const rows: Row[] = React.useMemo(() => {
    return coinMarketData.map((coinMarketItem: BraveWallet.CoinMarket) => {
      const buySupported = isBuySupported(coinMarketItem)
      const depositSupported = isDepositSupported(coinMarketItem)
      const buttonOptions = buySupported ? [BuyButtonOption, DepositButtonOption] : depositSupported ? [DepositButtonOption] : []
      return {
        id: `coin-row-${coinMarketItem.symbol}-${coinMarketItem.marketCapRank}`,
        content: renderCells(coinMarketItem, buttonOptions, fiatCurrency, onClickBuy, onClickDeposit),
        data: coinMarketItem,
        onClick: onSelectCoinMarket
      }
    })
  }, [coinMarketData, isBuySupported, isDepositSupported, onClickBuy, onClickDeposit])

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
