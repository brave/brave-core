// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { FixedSizeList } from 'react-window'

// types
import {
  BraveWallet,
  MarketGridColumnTypes,
  MarketGridRow,
  SortOrder,
  MarketGridHeader
} from '../../../constants/types'

// utils
import { createColumnTemplate, renderCells } from './market-grid-utils'
import { getLocale } from '../../../../common/locale'
import {
  BuyButtonOption,
  DepositButtonOption
} from '../../../options/account-list-button-options'

// styles
import {
  CoinGeckoText,
  Cell,
  GridContainer,
  Header,
  HeaderItem,
  GridRow,
  GridRowsWrapper,
  SortIcon,
  StyledWrapper,
  EmptyStateText
} from './market-grid.style'
import { Row } from '../style'

export type MarketGridProps = {
  headers: MarketGridHeader[]
  coinMarketData: BraveWallet.CoinMarket[]
  visibleRows?: number
  rowHeight?: number
  overScanCount?: number
  sortedBy?: MarketGridColumnTypes
  sortOrder?: SortOrder
  children?: React.ReactNode
  showEmptyState: boolean
  fiatCurrency: string
  onSort?: (columnId: MarketGridColumnTypes, sortOrder: SortOrder) => void
  onSelectCoinMarket: (coinMarket: BraveWallet.CoinMarket) => void
  isBuySupported: (coinMarket: BraveWallet.CoinMarket) => boolean
  isDepositSupported: (coinMarket: BraveWallet.CoinMarket) => boolean
  onClickBuy: (coinMarket: BraveWallet.CoinMarket) => void
  onClickDeposit: (coinMarket: BraveWallet.CoinMarket) => void
  onUpdateIframeHeight: (height: number) => void
}

const defaultVisibleRows = 15
const defaultRowHeight = 65
const defaultOverScanCount = 15

export const MarketGrid = ({
  headers,
  visibleRows = defaultVisibleRows,
  rowHeight = defaultRowHeight,
  overScanCount = defaultOverScanCount,
  sortedBy,
  sortOrder,
  coinMarketData,
  showEmptyState,
  fiatCurrency,
  onSort,
  onSelectCoinMarket,
  isBuySupported,
  isDepositSupported,
  onClickBuy,
  onClickDeposit,
  onUpdateIframeHeight
}: MarketGridProps) => {
  // state
  const [gridTemplateColumns, setGridTemplateColumns] = React.useState(
    createColumnTemplate(headers)
  )

  // refs
  const wrapperRef = React.useRef<HTMLDivElement>(null)

  // memos
  const rows: MarketGridRow[] = React.useMemo(() => {
    return coinMarketData.map((coinMarketItem: BraveWallet.CoinMarket) => {
      const buySupported = isBuySupported(coinMarketItem)
      const depositSupported = isDepositSupported(coinMarketItem)
      const buttonOptions = buySupported
        ? [BuyButtonOption, DepositButtonOption]
        : depositSupported
        ? [DepositButtonOption]
        : []
      return {
        id: `coin-row-${coinMarketItem.symbol}-${coinMarketItem.marketCapRank}`,
        content: renderCells(
          coinMarketItem,
          buttonOptions,
          fiatCurrency,
          onClickBuy,
          onClickDeposit
        ),
        data: coinMarketItem,
        onClick: onSelectCoinMarket
      }
    })
  }, [
    coinMarketData,
    fiatCurrency,
    isBuySupported,
    isDepositSupported,
    onClickBuy,
    onClickDeposit,
    onSelectCoinMarket
  ])

  // callbacks
  const onHeaderClick = React.useCallback(
    (header: MarketGridHeader) => {
      if (!header.sortable) return
      const newSortOrder: SortOrder =
        sortedBy === header.id ? (sortOrder === 'asc' ? 'desc' : 'asc') : 'desc'
      if (onSort) {
        onSort(header.id, newSortOrder)
      }
    },
    [sortedBy, sortOrder, onSort]
  )

  const renderRows = React.useCallback(
    ({ index, style }: { index: number; style: React.CSSProperties }) => {
      const row = rows[index]
      return (
        <GridRow
          key={row.id}
          templateColumns={gridTemplateColumns}
          style={style}
          onClick={() => row.onClick && row.onClick(row.data)}
        >
          {row.content.map((cell, cellIndex) => (
            <Cell
              key={cellIndex}
              hideOnSmall={headers[cellIndex].hideOnSmall}
              hideOnPanel={headers[cellIndex].hideOnPanel}
              style={cell.customStyle}
            >
              {cell.content}
            </Cell>
          ))}
        </GridRow>
      )
    },
    [rows, gridTemplateColumns, headers]
  )

  const onContentLoad = React.useCallback(() => {
    if (wrapperRef.current) {
      onUpdateIframeHeight(wrapperRef.current.scrollHeight)
    }
  }, [onUpdateIframeHeight, wrapperRef])

  React.useEffect(() => {
    const handleResize = () => {
      setGridTemplateColumns(createColumnTemplate(headers))
    }

    window.addEventListener('resize', handleResize)
    return () => {
      window.removeEventListener('resize', handleResize)
    }
  }, [headers])

  return (
    <StyledWrapper
      onLoad={onContentLoad}
      ref={wrapperRef}
    >
      <GridContainer>
        <Header templateColumns={gridTemplateColumns}>
          {headers.map((header) => (
            <HeaderItem
              key={header.id}
              hideOnSmall={header.hideOnSmall}
              hideOnPanel={header.hideOnPanel}
              onClick={() => onHeaderClick(header)}
              sortable={header.sortable}
              style={header.customStyles}
            >
              {header.label}
              {header.sortable && sortedBy === header.id && (
                <SortIcon
                  name={sortOrder === 'asc' ? 'sort-asc' : 'sort-desc'}
                />
              )}
            </HeaderItem>
          ))}
        </Header>
        {showEmptyState ? (
          <Row margin='30px 0px'>
            <EmptyStateText
              isBold={true}
              textSize='14px'
            >
              {getLocale('braveWalletMarketDataNoAssetsFound')}
            </EmptyStateText>
          </Row>
        ) : (
          <FixedSizeList
            height={visibleRows * rowHeight}
            itemCount={rows.length}
            itemSize={rowHeight}
            overscanCount={overScanCount}
            width='100%'
            outerElementType={GridRowsWrapper}
          >
            {renderRows}
          </FixedSizeList>
        )}
      </GridContainer>
      <CoinGeckoText>
        {getLocale('braveWalletPoweredByCoinGecko')}
      </CoinGeckoText>
    </StyledWrapper>
  )
}
