// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Utils
import {
  AccountButtonOptionsObjectType,
  BraveWallet,
  MarketGridHeader
} from '../../../constants/types'
import Amount from '../../../utils/amount'

// Styled components
import {
  TextWrapper,
  ButtonsRow,
  breakpoints,
  ActionButton
} from './market-grid.style'

// Components
import { AssetNameAndIcon } from '../../asset-name-and-icon'
import { AssetPriceChange } from '../../asset-price-change'
import { getLocale } from '../../../../common/locale'

// Render cells for a grid row
export const renderCells = (
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

  const formattedPrice = new Amount(currentPrice).formatAsFiat(fiatCurrency, 6)
  const formattedPercentageChange =
    new Amount(priceChangePercentage24h).value?.absoluteValue().toFixed(2) + '%'
  const formattedMarketCap = new Amount(marketCap).abbreviate(
    1,
    fiatCurrency,
    'billion'
  )
  const formattedVolume = new Amount(totalVolume).abbreviate(1, fiatCurrency)
  const isDown = priceChange24h < 0

  const cellsContent: React.ReactNode[] = [
    // Asset name and icon
    <AssetNameAndIcon
      assetName={name}
      symbol={symbol}
      assetLogo={image}
    />,

    // Price Column
    <TextWrapper alignment='left'>{formattedPrice}</TextWrapper>,

    // Price Change Column
    <TextWrapper alignment='left'>
      <AssetPriceChange
        isDown={isDown}
        priceChangePercentage={formattedPercentageChange}
      />
    </TextWrapper>,

    // Market Cap Column
    <TextWrapper alignment='left'>{formattedMarketCap}</TextWrapper>,

    // Volume Column
    <TextWrapper alignment='left'>{formattedVolume}</TextWrapper>,

    <ButtonsRow>
      {buttonOptions.map((option) => (
        <ActionButton
          key={option.id}
          onClick={
            option.id === 'buy'
              ? () => onClickBuy(coinMarkDataItem)
              : () => onClickDeposit(coinMarkDataItem)
          }
        >
          {getLocale(option.name)}
        </ActionButton>
      ))}
    </ButtonsRow>
  ]

  return cellsContent.map((cellContent) => ({ content: cellContent }))
}

/**
 * Creates a a string to be used as value for `grid-template-columns` for a grid
 * @param headers Grid headers
 * @returns
 */
export const createColumnTemplate = (headers: MarketGridHeader[]) => {
  let filteredHeaders = headers

  const mediaQueryPanel = window.matchMedia(`(max-width: ${breakpoints.panel})`)
  const mediaQuerySmall = window.matchMedia(
    `(min-width: ${breakpoints.panel}) and (max-width: ${breakpoints.small})`
  )

  if (mediaQueryPanel.matches) {
    filteredHeaders = headers.filter((header) => !header.hideOnPanel)
  } else if (mediaQuerySmall.matches) {
    filteredHeaders = headers.filter((header) => !header.hideOnSmall)
  }

  return filteredHeaders
    .map((header) =>
      header.width
        ? `${header.width} `
        : header.id === 'assets'
        ? '2.5fr'
        : '1fr'
    )
    .join(' ')
}
