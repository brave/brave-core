// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { useHistory } from 'react-router'

// Hooks
import {
  useGetCoinMarketQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetOnRampAssetsQuery
} from '../../../../common/slices/api.slice'

// Constants
import {
  BraveWallet,
  WalletRoutes,
  WalletState
} from '../../../../constants/types'

// Styled Components
import { LoadIcon, LoadIconWrapper, MarketDataIframe } from './style'

// Utils
import { WalletPageActions } from '../../../../page/actions'
import {
  braveMarketUiOrigin,
  MarketCommandMessage,
  MarketUiCommand,
  SelectCoinMarketMessage,
  SelectBuyMessage,
  SelectDepositMessage,
  sendMessageToMarketUiFrame,
  UpdateCoinMarketMessage,
  UpdateTradableAssetsMessage,
  UpdateBuyableAssetsMessage,
  UpdateDepositableAssetsMessage,
  UpdateIframeHeightMessage
} from '../../../../market/market-ui-messages'
import { makeDepositFundsRoute } from '../../../../utils/routes-utils'

const defaultCurrency = 'usd'
const assetsRequestLimit = 250

export const MarketView = () => {
  // State
  const [iframeLoaded, setIframeLoaded] = React.useState<boolean>(false)
  const [iframeHeight, setIframeHeight] = React.useState<number>(0)
  const marketDataIframeRef = React.useRef<HTMLIFrameElement>(null)

  // Redux
  const dispatch = useDispatch()
  const tradableAssets = useSelector(
    ({ wallet }: { wallet: WalletState }) => wallet.userVisibleTokensInfo
  )
  const fullTokenList = useSelector(
    ({ wallet }: { wallet: WalletState }) => wallet.fullTokenList
  )

  // Hooks
  const history = useHistory()

  // Queries
  const { data: defaultFiatCurrency = 'usd' } = useGetDefaultFiatCurrencyQuery()

  const { buyAssets } = useGetOnRampAssetsQuery(undefined, {
    selectFromResult: (res) => ({
      isLoading: res.isLoading,
      buyAssets: res.data?.allAssetOptions || []
    })
  })

  const { data: allCoins = [], isLoading: isLoadingCoinMarketData } =
    useGetCoinMarketQuery({
      vsAsset: defaultFiatCurrency,
      limit: assetsRequestLimit
    })

  // Methods
  const onSelectCoinMarket = React.useCallback(
    (coinMarket: BraveWallet.CoinMarket) => {
      dispatch(WalletPageActions.selectCoinMarket(coinMarket))
      history.push(`${WalletRoutes.Market}/${coinMarket.symbol}`)
    },
    []
  )

  const onSelectBuy = React.useCallback(
    (coinMarket: BraveWallet.CoinMarket) => {
      history.push(
        WalletRoutes.FundWalletPage.replace(':tokenId?', coinMarket.symbol)
      )
    },
    []
  )

  const onSelectDeposit = React.useCallback(
    (coinMarket: BraveWallet.CoinMarket) => {
      history.push(makeDepositFundsRoute(coinMarket.symbol))
    },
    []
  )

  const onMessageEventListener = React.useCallback(
    (event: MessageEvent<MarketCommandMessage>) => {
      // validate message origin
      if (event.origin !== braveMarketUiOrigin) return

      const message = event.data
      switch (message.command) {
        case MarketUiCommand.SelectCoinMarket: {
          const { payload } = message as SelectCoinMarketMessage
          onSelectCoinMarket(payload)
          break
        }

        case MarketUiCommand.SelectBuy: {
          const { payload } = message as SelectBuyMessage
          onSelectBuy(payload)
          break
        }

        case MarketUiCommand.SelectDeposit: {
          const { payload } = message as SelectDepositMessage
          onSelectDeposit(payload)
        }

        case MarketUiCommand.UpdateIframeHeight: {
          const { payload } = message as UpdateIframeHeightMessage
          setIframeHeight(payload)
        }
      }
    },
    [onSelectCoinMarket, onSelectBuy, onSelectDeposit]
  )

  const onMarketDataFrameLoad = React.useCallback(
    () => setIframeLoaded(true),
    []
  )

  // Effects
  React.useEffect(() => {
    if (!iframeLoaded || !marketDataIframeRef?.current) return

    const updateCoinsMsg: UpdateCoinMarketMessage = {
      command: MarketUiCommand.UpdateCoinMarkets,
      payload: {
        coins: allCoins,
        defaultFiatCurrency
      }
    }
    sendMessageToMarketUiFrame(
      marketDataIframeRef.current.contentWindow,
      updateCoinsMsg
    )

    const updateAssetsMsg: UpdateTradableAssetsMessage = {
      command: MarketUiCommand.UpdateTradableAssets,
      payload: tradableAssets
    }
    sendMessageToMarketUiFrame(
      marketDataIframeRef.current.contentWindow,
      updateAssetsMsg
    )

    const updateBuyableAssetsMsg: UpdateBuyableAssetsMessage = {
      command: MarketUiCommand.UpdateBuyableAssets,
      payload: buyAssets
    }
    sendMessageToMarketUiFrame(
      marketDataIframeRef.current.contentWindow,
      updateBuyableAssetsMsg
    )

    const updateDepositableAssetsMsg: UpdateDepositableAssetsMessage = {
      command: MarketUiCommand.UpdateDepositableAssets,
      payload: fullTokenList
    }
    sendMessageToMarketUiFrame(
      marketDataIframeRef.current.contentWindow,
      updateDepositableAssetsMsg
    )
  }, [
    iframeLoaded,
    marketDataIframeRef,
    allCoins,
    buyAssets,
    fullTokenList,
    defaultCurrency
  ])

  React.useEffect(() => {
    window.addEventListener('message', onMessageEventListener)
    return () => window.removeEventListener('message', onMessageEventListener)
  }, [])

  return (
    <>
      {isLoadingCoinMarketData ? (
        <LoadIconWrapper>
          <LoadIcon />
        </LoadIconWrapper>
      ) : (
        <MarketDataIframe
          iframeHeight={iframeHeight}
          ref={marketDataIframeRef}
          onLoad={onMarketDataFrameLoad}
          src='chrome-untrusted://market-display'
          sandbox='allow-scripts allow-same-origin'
        />
      )}
    </>
  )
}
