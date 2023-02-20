// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { BrowserRouter } from 'react-router-dom'
import { initLocale } from 'brave-ui'
import { loadTimeData } from '../../common/loadTimeData'

// css
import 'emptykit.css'
import 'chrome://resources/brave/fonts/poppins.css'
import './css/market.css'

// theme setup
import BraveCoreThemeProvider from '../../common/BraveCoreThemeProvider'
import walletDarkTheme from '../theme/wallet-dark'
import walletLightTheme from '../theme/wallet-light'

// constants
import {
  BraveWallet,
  DefaultCurrencies,
  MarketAssetFilterOption,
  MarketDataTableColumnTypes,
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
  UpdateTradableAssetsMessage

} from './market-ui-messages'
import { filterCoinMarkets, searchCoinMarkets, sortCoinMarkets } from '../utils/coin-market-utils'

// Options
import { AssetFilterOptions } from '../options/market-data-filter-options'
import { marketDataTableHeaders } from '../options/market-data-headers'

// components
import { MarketDataTable } from '../components/market-datatable'
import { TopRow } from '../components/desktop/views/market/style'
import { AssetsFilterDropdown } from '../components/desktop'
import { SearchBar } from '../components/shared'

const App = () => {
  // State
  const [tableHeaders, setTableHeaders] = React.useState(marketDataTableHeaders)
  const [currentFilter, setCurrentFilter] = React.useState<MarketAssetFilterOption>('all')
  const [sortOrder, setSortOrder] = React.useState<SortOrder>('desc')
  const [sortByColumnId, setSortByColumnId] = React.useState<MarketDataTableColumnTypes>('marketCap')
  const [searchTerm, setSearchTerm] = React.useState('')
  const [coinMarkets, setCoinMarkets] = React.useState<BraveWallet.CoinMarket[]>([])
  const [tradableAssets, setTradableAssets] = React.useState<BraveWallet.BlockchainToken[]>([])
  const [buyableAssets, setBuyableAssets] = React.useState<BraveWallet.BlockchainToken[]>([])
  const [depositableAssets, setDepositableAssets] = React.useState<BraveWallet.BlockchainToken[]>([])
  const [defaultCurrencies, setDefaultCurrencies] = React.useState<DefaultCurrencies>()

  // Memos
  const visibleCoinMarkets = React.useMemo(() => {
    const searchResults = searchTerm === '' ? coinMarkets : searchCoinMarkets(coinMarkets, searchTerm)
    const filteredCoins = filterCoinMarkets(searchResults, tradableAssets, currentFilter)
    return [...sortCoinMarkets(filteredCoins, sortOrder, sortByColumnId)]
  }, [coinMarkets, sortOrder, sortByColumnId, searchTerm, currentFilter])

  // Methods
  const isBuySupported = React.useCallback((coinMarket: BraveWallet.CoinMarket) => {
    return buyableAssets.some((asset) => asset.symbol.toLowerCase() === coinMarket.symbol.toLowerCase())
  }, [buyableAssets])

  const isDepositSupported = React.useCallback((coinMarket: BraveWallet.CoinMarket) => {
    return depositableAssets.some((asset) => asset.symbol.toLowerCase() === coinMarket.symbol.toLowerCase())
  }, [depositableAssets])

  const onClickBuy = React.useCallback((coinMarket: BraveWallet.CoinMarket) => {
    const message: SelectBuyMessage = {
      command: MarketUiCommand.SelectBuy,
      payload: coinMarket
    }
    sendMessageToWalletUi(parent, message)
  }, [])

  const onClickDeposit = React.useCallback((coinMarket: BraveWallet.CoinMarket) => {
    const message: SelectDepositMessage = {
      command: MarketUiCommand.SelectDeposit,
      payload: coinMarket
    }
    sendMessageToWalletUi(parent, message)
  }, [])

  const onSelectFilter = (value: MarketAssetFilterOption) => {
    setCurrentFilter(value)
  }

  const onMessageEventListener = React.useCallback((event: MessageEvent<MarketCommandMessage>) => {
    // validate message origin
    if (event.origin !== braveWalletOrigin) return

    const message = event.data
    switch (message.command) {
      case MarketUiCommand.UpdateCoinMarkets: {
        const { payload } = message as UpdateCoinMarketMessage
        setCoinMarkets(payload.coins)
        setDefaultCurrencies(payload.defaultCurrencies)
        break
      }

      case MarketUiCommand.UpdateTradableAssets: {
        const { payload } = message as UpdateTradableAssetsMessage
        setTradableAssets(payload)
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
  }, [])

  const onSort = React.useCallback((columnId: MarketDataTableColumnTypes, newSortOrder: SortOrder) => {
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
  }, [])

  const onSelectCoinMarket = React.useCallback((coinMarket: BraveWallet.CoinMarket) => {
    const message: SelectCoinMarketMessage = {
      command: MarketUiCommand.SelectCoinMarket,
      payload: coinMarket
    }
    sendMessageToWalletUi(parent, message)
  }, [])

  // Effects
  React.useEffect(() => {
    window.addEventListener('message', onMessageEventListener)
    return () => window.removeEventListener('message', onMessageEventListener)
  }, [])

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
              placeholder="Search"
              autoFocus={true}
              action={event => {
                setSearchTerm(event.target.value)
              }}
            />
          </TopRow>
          <MarketDataTable
            headers={tableHeaders}
            coinMarketData={visibleCoinMarkets}
            showEmptyState={searchTerm !== '' || currentFilter !== 'all'}
            fiatCurrency={defaultCurrencies?.fiat ?? 'USD'}
            onSelectCoinMarket={onSelectCoinMarket}
            onSort={onSort}
            isBuySupported={isBuySupported}
            isDepositSupported={isDepositSupported}
            onClickBuy={onClickBuy}
            onClickDeposit={onClickDeposit}
          />
        </>
      </BraveCoreThemeProvider>
    </BrowserRouter>
  )
}

function initialize () {
  initLocale(loadTimeData.data_)
  render(<App />, document.getElementById('mountPoint'))
}

document.addEventListener('DOMContentLoaded', initialize)
