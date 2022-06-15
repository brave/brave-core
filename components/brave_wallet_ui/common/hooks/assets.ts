// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

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
import { httpifyIpfsUrl } from '../../utils/string-utils'

const assetsLogo = (assets: BraveWallet.BlockchainToken[]) => {
  return assets.map(token => {
    let logo = token.logo
    if (token.logo?.startsWith('ipfs://')) {
      logo = httpifyIpfsUrl(token.logo)
    } else if (token.logo?.startsWith('data:image/')) {
      logo = token.logo
    } else {
      logo = `chrome://erc-token-images/${token.logo}`
    }

    return {
      ...token,
      logo
    } as BraveWallet.BlockchainToken
  })
}

export function useAssets () {
  // redux
  const {
    selectedAccount,
    networkList,
    selectedNetwork,
    userVisibleTokensInfo,
    transactionSpotPrices: spotPrices
  } = useSelector((state: { wallet: WalletState }) => state.wallet)

  const isMounted = useIsMounted()
  const { getBuyAssets } = useLib()

  const { computeFiatAmount } = usePricing(spotPrices)
  const getBalance = useBalance(networkList)

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

  const [wyreAssetOptions, setWyreAssetOptions] = React.useState<BraveWallet.BlockchainToken[]>([])
  const [rampAssetOptions, setRampAssetOptions] = React.useState<BraveWallet.BlockchainToken[]>([])
  const [sardineAssetOptions, setSardineAssetOptions] = React.useState<BraveWallet.BlockchainToken[]>([])

  React.useEffect(() => {
    // Prevent calling getBuyAssets if the selectedNetwork is
    // not supported.
    if (!BuySupportedChains.includes(selectedNetwork.chainId)) {
      return
    }

    const fetchTokens = async () => {
      const registryTokens = await Promise.all([
        getBuyAssets(BraveWallet.OnRampProvider.kWyre, selectedNetwork.chainId),
        getBuyAssets(BraveWallet.OnRampProvider.kRamp, selectedNetwork.chainId),
        getBuyAssets(BraveWallet.OnRampProvider.kSardine, selectedNetwork.chainId)
      ])

      const wyreAssetOptions = assetsLogo(registryTokens[0])
      const rampAssetOptions = assetsLogo(registryTokens[1])
      const sardineAssetOptions = assetsLogo(registryTokens[2])

      if (isMounted) {
        setWyreAssetOptions(wyreAssetOptions)
        setRampAssetOptions(rampAssetOptions)
        setSardineAssetOptions(sardineAssetOptions)
      }
    }

    fetchTokens()
      .catch(e => {
        console.error(e)
      })
  }, [selectedNetwork])

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
    return [...rampAssetOptions, ...wyreAssetOptions, ...sardineAssetOptions].filter(asset => asset.chainId === selectedNetwork.chainId)
  }, [rampAssetOptions, wyreAssetOptions, sardineAssetOptions, selectedNetwork])

  const buyAssetOptionsAllChains = React.useMemo(() => {
    return [...rampAssetOptions, ...wyreAssetOptions]
  }, [rampAssetOptions, wyreAssetOptions, selectedNetwork])

  return {
    sendAssetOptions: assetsByNetwork,
    buyAssetOptions,
    buyAssetOptionsAllChains,
    rampAssetOptions,
    wyreAssetOptions,
    sardineAssetOptions,
    panelUserAssetList: assetsByValueAndNetwork
  }
}

export default useAssets
