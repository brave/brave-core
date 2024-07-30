// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// Hooks
import {
  useGetCoinMarketQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetOnRampAssetsQuery
} from '../../../../common/slices/api.slice'
import {
  useGetCombinedTokensListQuery //
} from '../../../../common/slices/api.slice.extra'

// Constants
import { WalletRoutes } from '../../../../constants/types'

// Styled Components
import { LoadIcon, LoadIconWrapper, MarketDataIframe } from './style'
import { Column } from '../../../shared/style'

// Utils
import {
  braveMarketUiOrigin,
  MarketCommandMessage,
  MarketUiCommand,
  SelectCoinMarketMessage,
  SelectBuyMessage,
  SelectDepositMessage,
  sendMessageToMarketUiFrame,
  UpdateCoinMarketMessage,
  UpdateBuyableAssetsMessage,
  UpdateDepositableAssetsMessage,
  UpdateIframeHeightMessage
} from '../../../../market/market-ui-messages'
import {
  makeDepositFundsRoute,
  makeFundWalletRoute
} from '../../../../utils/routes-utils'
import { getAssetIdKey } from '../../../../utils/asset-utils'

const assetsRequestLimit = 250

export const MarketView = () => {
  // refs
  const isMountedRef = React.useRef(true)

  // State
  const [iframeLoaded, setIframeLoaded] = React.useState<boolean>(false)
  const [iframeHeight, setIframeHeight] = React.useState<number>(0)
  const marketDataIframeRef = React.useRef<HTMLIFrameElement>(null)

  // Hooks
  const history = useHistory()

  // Queries
  const { data: defaultFiatCurrency = 'usd' } = useGetDefaultFiatCurrencyQuery()
  const { data: combinedTokensList } = useGetCombinedTokensListQuery()

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
  const onMessageEventListener = React.useCallback(
    (event: MessageEvent<MarketCommandMessage>) => {
      // validate message origin
      if (event.origin !== braveMarketUiOrigin) return

      const message = event.data
      switch (message.command) {
        case MarketUiCommand.SelectCoinMarket: {
          const { payload } = message as SelectCoinMarketMessage
          history.push(`${WalletRoutes.Market}/${payload.id}`)
          break
        }

        case MarketUiCommand.SelectBuy: {
          const { payload } = message as SelectBuyMessage
          const symbolLower = payload.symbol.toLowerCase()
          const foundTokens = buyAssets.filter(
            (t) => t.symbol.toLowerCase() === symbolLower
          )

          if (foundTokens.length === 1) {
            history.push(
              makeFundWalletRoute(getAssetIdKey(foundTokens[0]), {
                searchText: symbolLower
              })
            )
            return
          }

          if (foundTokens.length > 1) {
            history.push(
              makeFundWalletRoute('', {
                searchText: symbolLower
              })
            )
          }
          break
        }

        case MarketUiCommand.SelectDeposit: {
          const { payload } = message as SelectDepositMessage
          const symbolLower = payload.symbol.toLowerCase()
          const foundTokens = combinedTokensList.filter(
            (t) => t.symbol.toLowerCase() === symbolLower
          )

          if (foundTokens.length === 1) {
            history.push(
              makeDepositFundsRoute(getAssetIdKey(foundTokens[0]), {
                searchText: symbolLower
              })
            )
            return
          }

          if (foundTokens.length > 1) {
            history.push(
              makeDepositFundsRoute('', {
                searchText: symbolLower
              })
            )
          }
        }

        case MarketUiCommand.UpdateIframeHeight: {
          const { payload } = message as UpdateIframeHeightMessage
          // prevent memory-leaks
          if (!isMountedRef.current) return
          setIframeHeight(payload)
        }
      }
    },
    [buyAssets, combinedTokensList, history]
  )

  const onMarketDataFrameLoad = React.useCallback(() => {
    if (isMountedRef.current) {
      setIframeLoaded(true)
    }
  }, [isMountedRef])

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
      payload: combinedTokensList
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
    combinedTokensList,
    defaultFiatCurrency
  ])

  React.useEffect(() => {
    isMountedRef.current = true
    window.addEventListener('message', onMessageEventListener)

    return () => {
      isMountedRef.current = false
      window.removeEventListener('message', onMessageEventListener)
    }
  }, [onMessageEventListener])

  return (
    <>
      {isLoadingCoinMarketData ? (
        <LoadIconWrapper>
          <LoadIcon />
        </LoadIconWrapper>
      ) : (
        <Column
          fullHeight
          fullWidth
        >
          <MarketDataIframe
            iframeHeight={iframeHeight}
            ref={marketDataIframeRef}
            onLoad={onMarketDataFrameLoad}
            src='chrome-untrusted://market-display'
            sandbox='allow-scripts allow-same-origin'
          />
        </Column>
      )}
    </>
  )
}
