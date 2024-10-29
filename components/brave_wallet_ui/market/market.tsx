// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { BrowserRouter } from 'react-router-dom'
import { initLocale } from 'brave-ui'
import { loadTimeData } from '../../common/loadTimeData'

// css
import 'emptykit.css'
import './css/market.css'

// theme setup
import BraveCoreThemeProvider from '../../common/BraveCoreThemeProvider'
import walletDarkTheme from '../theme/wallet-dark'
import walletLightTheme from '../theme/wallet-light'

// leo icons setup
import { setIconBasePath } from '@brave/leo/react/icon'
setIconBasePath('chrome-untrusted://resources/brave-icons')

// constants
import {
  BraveWallet,
  MarketAssetFilterOption,
  MarketGridColumnTypes,
  MeldCryptoCurrency,
  SortOrder
} from '../constants/types'

// utils
import {
  braveWalletOrigin,
  MarketCommandMessage,
  MarketUiCommand,
  SelectCoinMarketMessage,
  SelectBuyMessage,
  SelectDepositMessage,
  sendMessageToWalletUi,
  UpdateBuyableAssetsMessage,
  UpdateDepositableAssetsMessage,
  UpdateCoinMarketMessage,
  UpdateIframeHeightMessage,
  braveWalletPanelOrigin
} from './market-ui-messages'
import {
  filterCoinMarkets,
  searchCoinMarkets,
  sortCoinMarkets
} from '../utils/coin-market-utils'
import { getAssetSymbol } from '../utils/meld_utils'

// Options
import { AssetFilterOptions } from '../options/market-data-filter-options'
import { marketGridHeaders } from '../options/market-data-headers'

// components
import { TopRow } from '../components/desktop/views/market/style'
import {
  AssetsFilterDropdown //
} from '../components/desktop/assets-filter-dropdown/index'
import { SearchBar } from '../components/shared/search-bar/index'
import { MarketGrid } from '../components/shared/market-grid/market-grid'

const App = () => {
  // State
  const [currentFilter, setCurrentFilter] =
    React.useState<MarketAssetFilterOption>('all')
  const [sortOrder, setSortOrder] = React.useState<SortOrder>('desc')
  const [sortByColumnId, setSortByColumnId] =
    React.useState<MarketGridColumnTypes>('marketCap')
  const [searchTerm, setSearchTerm] = React.useState('')
  const [coinMarkets, setCoinMarkets] = React.useState<
    BraveWallet.CoinMarket[]
  >([])
  const [buyableAssets, setBuyableAssets] = React.useState<
    MeldCryptoCurrency[] | undefined
  >([])
  const [depositableAssets, setDepositableAssets] = React.useState<
    BraveWallet.BlockchainToken[]
  >([])
  const [defaultFiatCurrency, setDefaultFiatCurrency] = React.useState<string>()

  // Constants
  const origin =
    window.location.ancestorOrigins[0] === braveWalletPanelOrigin
      ? braveWalletPanelOrigin
      : braveWalletOrigin

  // Memos
  const visibleCoinMarkets = React.useMemo(() => {
    const searchResults =
      searchTerm === ''
        ? coinMarkets
        : searchCoinMarkets(coinMarkets, searchTerm)
    const filteredCoins = filterCoinMarkets(
      searchResults,
      buyableAssets,
      currentFilter
    )
    return [...sortCoinMarkets(filteredCoins, sortOrder, sortByColumnId)]
  }, [
    searchTerm,
    coinMarkets,
    buyableAssets,
    currentFilter,
    sortOrder,
    sortByColumnId
  ])

  // Methods
  const isBuySupported = React.useCallback(
    (coinMarket: BraveWallet.CoinMarket) => {
      return (
        buyableAssets?.some(
          (asset) =>
            getAssetSymbol(asset).toLowerCase() ===
            coinMarket.symbol.toLowerCase()
        ) ?? false
      )
    },
    [buyableAssets]
  )

  const isDepositSupported = React.useCallback(
    (coinMarket: BraveWallet.CoinMarket) => {
      return depositableAssets.some(
        (asset) =>
          asset.symbol.toLowerCase() === coinMarket.symbol.toLowerCase()
      )
    },
    [depositableAssets]
  )

  const onClickBuy = React.useCallback(
    (coinMarket: BraveWallet.CoinMarket) => {
      const message: SelectBuyMessage = {
        command: MarketUiCommand.SelectBuy,
        payload: coinMarket
      }
      sendMessageToWalletUi(parent, message, origin)
    },
    [origin]
  )

  const onClickDeposit = React.useCallback(
    (coinMarket: BraveWallet.CoinMarket) => {
      const message: SelectDepositMessage = {
        command: MarketUiCommand.SelectDeposit,
        payload: coinMarket
      }
      sendMessageToWalletUi(parent, message, origin)
    },
    [origin]
  )

  const onUpdateIframeHeight = React.useCallback(
    (height: number) => {
      const message: UpdateIframeHeightMessage = {
        command: MarketUiCommand.UpdateIframeHeight,
        payload: height
      }
      sendMessageToWalletUi(parent, message, origin)
    },
    [origin]
  )

  const onSelectFilter = (value: MarketAssetFilterOption) => {
    setCurrentFilter(value)
  }

  const onMessageEventListener = React.useCallback(
    (event: MessageEvent<MarketCommandMessage>) => {
      // validate message origin
      if (
        event.origin === braveWalletOrigin ||
        event.origin === braveWalletPanelOrigin
      ) {
        const message = event.data
        switch (message.command) {
          case MarketUiCommand.UpdateCoinMarkets: {
            const { payload } = message as UpdateCoinMarketMessage
            setCoinMarkets(payload.coins)
            setDefaultFiatCurrency(payload.defaultFiatCurrency)
            break
          }

          case MarketUiCommand.UpdateBuyableAssets: {
            const { payload } = message as UpdateBuyableAssetsMessage
            setBuyableAssets(payload)
            break
          }

          case MarketUiCommand.UpdateDepositableAssets: {
            const { payload } = message as UpdateDepositableAssetsMessage
            setDepositableAssets(payload)
          }
        }
      }
    },
    []
  )

  const onSort = React.useCallback(
    (columnId: MarketGridColumnTypes, newSortOrder: SortOrder) => {
      setSortByColumnId(columnId)
      setSortOrder(newSortOrder)
    },
    []
  )

  const onSelectCoinMarket = React.useCallback(
    (coinMarket: BraveWallet.CoinMarket) => {
      const message: SelectCoinMarketMessage = {
        command: MarketUiCommand.SelectCoinMarket,
        payload: coinMarket
      }
      sendMessageToWalletUi(parent, message, origin)
    },
    [origin]
  )

  // Effects
  React.useEffect(() => {
    window.addEventListener('message', onMessageEventListener)
    return () => window.removeEventListener('message', onMessageEventListener)
  }, [onMessageEventListener])

  return (
    <BrowserRouter>
      <BraveCoreThemeProvider
        dark={walletDarkTheme}
        light={walletLightTheme}
      >
        <>
          <TopRow>
            <AssetsFilterDropdown
              options={AssetFilterOptions}
              value={currentFilter}
              onSelectFilter={onSelectFilter}
            />
            <SearchBar
              placeholder='Search'
              autoFocus={true}
              action={(event) => {
                setSearchTerm(event.target.value)
              }}
              isV2={true}
            />
          </TopRow>
          <MarketGrid
            headers={marketGridHeaders}
            coinMarketData={visibleCoinMarkets}
            showEmptyState={visibleCoinMarkets.length === 0}
            fiatCurrency={defaultFiatCurrency || 'USD'}
            sortedBy={sortByColumnId}
            sortOrder={sortOrder}
            onSelectCoinMarket={onSelectCoinMarket}
            onSort={onSort}
            isBuySupported={isBuySupported}
            isDepositSupported={isDepositSupported}
            onClickBuy={onClickBuy}
            onClickDeposit={onClickDeposit}
            onUpdateIframeHeight={onUpdateIframeHeight}
          />
        </>
      </BraveCoreThemeProvider>
    </BrowserRouter>
  )
}

function initialize() {
  initLocale(loadTimeData.data_)
  const root = createRoot(document.getElementById('mountPoint')!)
  root.render(<App />)
}

document.addEventListener('DOMContentLoaded', initialize)
