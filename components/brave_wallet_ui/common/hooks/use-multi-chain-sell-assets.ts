// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getNetworkInfo } from '../../utils/network-utils'
import {
  filterTokensByNetworks,
  getRampAssetSymbol
} from '../../utils/asset-utils'
import Amount from '../../utils/amount'

// Types
import { BraveWallet } from '../../constants/types'
import { WalletSelectors } from '../selectors'

// Hooks
import { useIsMounted } from './useIsMounted'
import { useLib } from './useLib'
import { useUnsafeWalletSelector } from './use-safe-selector'
import { useGetOffRampNetworksQuery } from '../slices/api.slice'

export const useMultiChainSellAssets = () => {
  // Redux
  const defaultCurrencies = useUnsafeWalletSelector(WalletSelectors.defaultCurrencies)

  // Queries
  const { data: offRampNetworks = [] } = useGetOffRampNetworksQuery()

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
  const selectedSellAssetNetwork = React.useMemo((): BraveWallet.NetworkInfo | undefined => {
    if (selectedSellAsset?.chainId && selectedSellAsset?.coin) {
      return getNetworkInfo(
        selectedSellAsset.chainId,
        selectedSellAsset.coin,
        offRampNetworks
      )
    }
    return undefined
  }, [selectedSellAsset?.chainId, selectedSellAsset?.coin, offRampNetworks])

  // Methods
  const getAllSellAssetOptions = React.useCallback(() => {
    getAllSellAssets()
      .then(result => {
        if (isMounted && result) {
          setOptions({
            rampAssetOptions: filterTokensByNetworks(
              result.rampAssetOptions,
              offRampNetworks
            ),
            allAssetOptions: filterTokensByNetworks(
              result.allAssetOptions,
              offRampNetworks
            )
          })
        }
      }).catch(e => console.error(e))
  }, [getAllSellAssets, offRampNetworks, isMounted])

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
      amount: sellAsset.coin === BraveWallet.CoinType.SOL
        ? new Amount(sellAmount).multiplyByDecimals(sellAsset?.decimals ?? 18).toNumber().toString()
        : new Amount(sellAmount).multiplyByDecimals(sellAsset?.decimals ?? 18).toHex(),
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
