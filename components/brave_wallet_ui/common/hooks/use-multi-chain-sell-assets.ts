// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getNetworkInfo } from '../../utils/network-utils'
import {
  getRampAssetSymbol
} from '../../utils/asset-utils'

// Types
import {
  BraveWallet,
  SupportedOffRampNetworks
} from '../../constants/types'
import { WalletSelectors } from '../selectors'

// Hooks
import { useIsMounted } from './useIsMounted'
import { useLib } from './useLib'
import { useUnsafeWalletSelector } from './use-safe-selector'

export const useMultiChainSellAssets = () => {
  // Redux
  const networkList = useUnsafeWalletSelector(WalletSelectors.networkList)
  const defaultCurrencies = useUnsafeWalletSelector(WalletSelectors.defaultCurrencies)

  // Hooks
  const isMounted = useIsMounted()
  const { getAllSellAssets, getSellAssetUrl } = useLib()

  // State
  const [sellAmount, setSellAmount] = React.useState<string>('')
  const [selectedSellAsset, setSelectedSellAsset] = React.useState<BraveWallet.BlockchainToken>()
  const [options, setOptions] = React.useState<
    {
      rampAssetOptions: BraveWallet.BlockchainToken[]
      allAssetOptions: BraveWallet.BlockchainToken[]
    }
  >({
    rampAssetOptions: [],
    allAssetOptions: []
  })

  // Memos
  const supportedSellAssetNetworks = React.useMemo((): BraveWallet.NetworkInfo[] => {
    return networkList.filter(n =>
      SupportedOffRampNetworks.includes(n.chainId)
    )
  }, [networkList])

  const selectedSellAssetNetwork = React.useMemo((): BraveWallet.NetworkInfo | undefined => {
    if (selectedSellAsset?.chainId && selectedSellAsset?.coin) {
      return getNetworkInfo(selectedSellAsset.chainId, selectedSellAsset.coin, supportedSellAssetNetworks)
    }
    return undefined
  }, [selectedSellAsset?.chainId, selectedSellAsset?.coin, supportedSellAssetNetworks])

  // Methods
  const getAllSellAssetOptions = React.useCallback(() => {
    getAllSellAssets()
      .then(result => {
        if (isMounted && result) {
          setOptions(result)
        }
      }).catch(e => console.error(e))
  }, [getAllSellAssets, isMounted])

  const openSellAssetLink = React.useCallback(({ sellAddress, sellAsset }: {
    sellAddress: string
    sellAsset: BraveWallet.BlockchainToken | undefined
  }) => {
    if (!sellAsset) {
      return
    }

    const asset = {
      ...sellAsset,
      symbol: getRampAssetSymbol(sellAsset, true)
    }

    getSellAssetUrl({
      asset: asset,
      offRampProvider: BraveWallet.OffRampProvider.kRamp,
      chainId: sellAsset.chainId,
      address: sellAddress,
      amount: sellAmount,
      currencyCode: defaultCurrencies.fiat
    })
      .then(url => {
        chrome.tabs.create({ url }, () => {
          if (chrome.runtime.lastError) {
            console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
          }
        })
      })
      .catch(e => console.error(e))
  }, [
    getSellAssetUrl,
    defaultCurrencies.fiat,
    sellAmount
  ])

  const checkIsAssetSellSupported = React.useCallback((token: BraveWallet.BlockchainToken) => {
    return options.allAssetOptions.some(
      (asset) =>
        (asset.symbol.toLowerCase() === token.symbol.toLowerCase() ||
          asset.contractAddress.toLowerCase() === token.contractAddress.toLowerCase()) &&
        asset.chainId === token.chainId)
  }, [options.allAssetOptions])

  return {
    allSellAssetOptions: options.allAssetOptions,
    getAllSellAssetOptions,
    selectedSellAsset,
    setSelectedSellAsset,
    selectedSellAssetNetwork,
    sellAmount,
    setSellAmount,
    openSellAssetLink,
    checkIsAssetSellSupported
  }
}
