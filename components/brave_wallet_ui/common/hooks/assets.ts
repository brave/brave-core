// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

// utils
import { addLogoToToken, isSardineSupported } from '../../utils/asset-utils'

// Constants
import {
  BraveWallet,
  WalletState,
  BuySupportedChains
} from '../../constants/types'

// Hooks
import usePricing from './pricing'
import useBalance from './balance'
import { useIsMounted } from './useIsMounted'
import { useLib } from './useLib'

export function useAssets () {
  // redux
  const {
    selectedAccount,
    networkList,
    selectedNetwork,
    userVisibleTokensInfo,
    transactionSpotPrices: spotPrices
  } = useSelector((state: { wallet: WalletState }) => state.wallet)

  // custom hooks
  const isMounted = useIsMounted()
  const { getBuyAssets } = useLib()
  const { computeFiatAmount } = usePricing(spotPrices)
  const getBalance = useBalance(networkList)

  // state
  const [wyreAssetOptions, setWyreAssetOptions] = React.useState<BraveWallet.BlockchainToken[]>([])
  const [rampAssetOptions, setRampAssetOptions] = React.useState<BraveWallet.BlockchainToken[]>([])
  const [sardineAssetOptions, setSardineAssetOptions] = React.useState<BraveWallet.BlockchainToken[]>([])

  // memos
  const assetsByNetwork = React.useMemo(() => {
    if (!userVisibleTokensInfo) {
      return []
    }
    // We also filter by coinType here because localhost
    // networks share the same chainId.
    return userVisibleTokensInfo.filter((token) =>
      token.chainId === selectedNetwork.chainId &&
      token.coin === selectedNetwork.coin
    )
  }, [userVisibleTokensInfo, selectedNetwork])

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

  const buyAssetOptions = React.useMemo(() => {
    const assetOptions = isSardineSupported()
      ? [...rampAssetOptions, ...wyreAssetOptions, ...sardineAssetOptions]
      : [...rampAssetOptions, ...wyreAssetOptions]

    return assetOptions
      .filter(asset => asset.chainId === selectedNetwork.chainId)
  }, [rampAssetOptions, wyreAssetOptions, sardineAssetOptions, selectedNetwork])

  // methods
  const getAllBuyOptionsCurrentNetwork = React.useCallback(async () => {
    // Prevent calling getBuyAssets if the selectedNetwork is not supported.
    if (!BuySupportedChains.includes(selectedNetwork.chainId)) {
      return
    }

    const registryTokens = await Promise.all([
      getBuyAssets(BraveWallet.OnRampProvider.kWyre, selectedNetwork.chainId),
      getBuyAssets(BraveWallet.OnRampProvider.kRamp, selectedNetwork.chainId),
      getBuyAssets(BraveWallet.OnRampProvider.kSardine, selectedNetwork.chainId)
    ])

    const wyreAssetOptions = registryTokens[0].map(addLogoToToken)
    const rampAssetOptions = registryTokens[1].map(addLogoToToken)
    const sardineAssetOptions = registryTokens[2].map(addLogoToToken)

    if (isMounted) {
      setWyreAssetOptions(wyreAssetOptions)
      setRampAssetOptions(rampAssetOptions)
      setSardineAssetOptions(sardineAssetOptions)
    }
  }, [getBuyAssets, selectedNetwork, isMounted])

  // effects
  React.useEffect(() => {
    getAllBuyOptionsCurrentNetwork()
  }, [getAllBuyOptionsCurrentNetwork])

  return {
    sendAssetOptions: assetsByNetwork,
    buyAssetOptions,
    rampAssetOptions,
    wyreAssetOptions,
    sardineAssetOptions,
    panelUserAssetList: assetsByValueAndNetwork,
    getAllBuyOptionsCurrentNetwork
  }
}

export default useAssets
