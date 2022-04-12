// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

// Constants
import {
  BraveWallet,
  WalletState
} from '../../constants/types'

// Hooks
import usePricing from './pricing'
import useBalance from './balance'
import { useIsMounted } from './useIsMounted'
import { useLib } from './useLib'
import { useSelectedNetworkNativeAsset } from './useSelectedNetworkNativeAsset'

export const useAssets = () => {
  // redux
  const {
    selectedAccount,
    networkList,
    selectedNetwork,
    userVisibleTokensInfo
  } = useSelector((state: { wallet: WalletState }) => state.wallet)

  const isMounted = useIsMounted()
  const { getBuyAssets } = useLib()

  const { computeFiatAmount } = usePricing()
  const getBalance = useBalance(networkList)
  const nativeAsset = useSelectedNetworkNativeAsset()

  const assetsByNetwork = React.useMemo(() => {
    if (!userVisibleTokensInfo) {
      return []
    }

    return userVisibleTokensInfo.filter((token) => token.chainId === selectedNetwork.chainId)
  }, [userVisibleTokensInfo, selectedNetwork])

  const [buyAssetOptions, setBuyAssetOptions] = React.useState<BraveWallet.BlockchainToken[]>([nativeAsset])

  React.useEffect(() => {
    isMounted && getBuyAssets().then(tokens => {
      if (isMounted) {
        setBuyAssetOptions(tokens.map(token => ({
          ...token,
          logo: `chrome://erc-token-images/${token.logo}`
        }) as BraveWallet.BlockchainToken))
      }
    }).catch(e => console.error(e))
  }, [])

  const assetsByValueAndNetwork = React.useMemo(() => {
    if (!assetsByNetwork) {
      return []
    }

    if (!selectedAccount) {
      return []
    }

    return assetsByNetwork.sort(function (a, b) {
      const aBalance = getBalance(selectedAccount, a)
      const bBalance = getBalance(selectedAccount, b)

      const bFiatBalance = computeFiatAmount(bBalance, b.symbol, b.decimals)
      const aFiatBalance = computeFiatAmount(aBalance, a.symbol, a.decimals)

      return bFiatBalance.toNumber() - aFiatBalance.toNumber()
    })
  }, [selectedAccount, assetsByNetwork, getBalance, computeFiatAmount])

  return {
    sendAssetOptions: assetsByNetwork,
    buyAssetOptions,
    panelUserAssetList: assetsByValueAndNetwork
  }
}

export default useAssets
