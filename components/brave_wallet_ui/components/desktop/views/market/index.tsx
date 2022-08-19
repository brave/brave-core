// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { useHistory } from 'react-router'

// Constants
import { BraveWallet, WalletRoutes, WalletState } from '../../../../constants/types'

// Actions
import { WalletActions } from '../../../../common/actions'

// Styled Components
import { LoadIcon, LoadIconWrapper, MarketDataIframe, StyledWrapper } from './style'

// Utils
import { WalletPageActions } from '../../../../page/actions'
import {
  braveMarketUiOrigin,
  MarketCommandMessage,
  MarketUiCommand,
  SelectCoinMarketMessage,
  sendMessageToMarketUiFrame,
  UpdateCoinMarketMessage,
  UpdateTradableAssetsMessage
} from '../../../../market/market-ui-messages'

const defaultCurrency = 'usd'
const assetsRequestLimit = 250

export const MarketView = () => {
  const [iframeLoaded, setIframeLoaded] = React.useState<boolean>(false)
  const marketDataIframeRef = React.useRef<HTMLIFrameElement>(null)

  // Redux
  const dispatch = useDispatch()
  const {
    isLoadingCoinMarketData,
    coinMarketData: allCoins,
    userVisibleTokensInfo: tradableAssets
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  // Hooks
  const history = useHistory()

  const onSelectCoinMarket = React.useCallback((coinMarket: BraveWallet.CoinMarket) => {
    dispatch(WalletPageActions.selectCoinMarket(coinMarket))
    history.push(`${WalletRoutes.Market}/${coinMarket.symbol}`)
  }, [])

  const onMessageEventListener = React.useCallback((event: MessageEvent<MarketCommandMessage>) => {
    // validate message origin
    if (event.origin !== braveMarketUiOrigin) return

    const message = event.data
    const { payload } = message as SelectCoinMarketMessage
    onSelectCoinMarket(payload)
  }, [])

  const onMarketDataFrameLoad = React.useCallback(() => setIframeLoaded(true), [])

  React.useEffect(() => {
    if (allCoins.length === 0) {
      dispatch(WalletActions.getCoinMarkets({
        vsAsset: defaultCurrency,
        limit: assetsRequestLimit
      }))
    }
  }, [allCoins])

  React.useEffect(() => {
    if (!iframeLoaded || !marketDataIframeRef?.current) return

    const updateCoinsMsg: UpdateCoinMarketMessage = {
      command: MarketUiCommand.UpdateCoinMarkets,
      payload: allCoins
    }
    sendMessageToMarketUiFrame(marketDataIframeRef.current.contentWindow, updateCoinsMsg)

    const updateAssetsMsg: UpdateTradableAssetsMessage = {
      command: MarketUiCommand.UpdateTradableAssets,
      payload: tradableAssets
    }
    sendMessageToMarketUiFrame(marketDataIframeRef.current.contentWindow, updateAssetsMsg)
  }, [iframeLoaded, marketDataIframeRef, allCoins])

  React.useEffect(() => {
    window.addEventListener('message', onMessageEventListener)
    return () => window.removeEventListener('message', onMessageEventListener)
  }, [])

  return (
    <StyledWrapper>
      {isLoadingCoinMarketData
        ? <LoadIconWrapper>
          <LoadIcon />
        </LoadIconWrapper>
        : <MarketDataIframe
          ref={marketDataIframeRef}
          onLoad={onMarketDataFrameLoad}
          src="chrome-untrusted://market-display"
          sandbox="allow-scripts allow-same-origin"
        />
      }
    </StyledWrapper>
  )
}
