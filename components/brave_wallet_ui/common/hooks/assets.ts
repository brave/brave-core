// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Constants
import {
  BraveWallet,
  WalletAccountType
} from '../../constants/types'

// Options
import { makeNetworkAsset } from '../../options/asset-options'

// Hooks
import usePricing from './pricing'
import useBalance from './balance'

export default function useAssets (
  selectedAccount: WalletAccountType,
  networkList: BraveWallet.NetworkInfo[],
  selectedNetwork: BraveWallet.NetworkInfo,
  fullTokenList: BraveWallet.BlockchainToken[],
  userVisibleTokensInfo: BraveWallet.BlockchainToken[],
  spotPrices: BraveWallet.AssetPrice[],
  getBuyAssets: () => Promise<BraveWallet.BlockchainToken[]>
) {
  const { computeFiatAmount } = usePricing(spotPrices)
  const getBalance = useBalance(networkList)
  const nativeAsset = React.useMemo(
    () => makeNetworkAsset(selectedNetwork),
    [selectedNetwork]
  )

  const assetsByNetwork = React.useMemo(() => {
    if (!userVisibleTokensInfo) {
      return []
    }

    return userVisibleTokensInfo.filter((token) => token.chainId === selectedNetwork.chainId)
  }, [userVisibleTokensInfo, selectedNetwork])

  const swapAssetOptions: BraveWallet.BlockchainToken[] = React.useMemo(() => {
    return [
      nativeAsset,
      ...fullTokenList.filter((asset) => asset.symbol.toUpperCase() === 'BAT'),
      ...assetsByNetwork
        .filter(asset => !['BAT', nativeAsset.symbol.toUpperCase()].includes(asset.symbol.toUpperCase())),
      ...fullTokenList
        .filter(asset => !['BAT', nativeAsset.symbol.toUpperCase()].includes(asset.symbol.toUpperCase()))
        .filter(asset => !assetsByNetwork
          .some(token => token.symbol.toUpperCase() === asset.symbol.toUpperCase()))
    ]
  }, [fullTokenList, userVisibleTokensInfo, nativeAsset, assetsByNetwork])

  const [buyAssetOptions, setBuyAssetOptions] = React.useState<BraveWallet.BlockchainToken[]>([nativeAsset])

  React.useEffect(() => {
    getBuyAssets().then(tokens => {
      setBuyAssetOptions(tokens.map(token => ({
        ...token,
        logo: `chrome://erc-token-images/${token.logo}`
      }) as BraveWallet.BlockchainToken))
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
    swapAssetOptions,
    sendAssetOptions: assetsByNetwork,
    buyAssetOptions,
    panelUserAssetList: assetsByValueAndNetwork
  }
}
