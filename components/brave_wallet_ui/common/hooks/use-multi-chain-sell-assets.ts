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
import { BraveWallet, CoinType } from '../../constants/types'

// Hooks
import { useLib } from './useLib'
import {
  useGetDefaultFiatCurrencyQuery,
  useGetOffRampNetworksQuery,
  useGetOffRampAssetsQuery
} from '../slices/api.slice'

export const useMultiChainSellAssets = () => {
  // Queries
  const { data: offRampNetworks = [] } = useGetOffRampNetworksQuery()
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
  const { data: { allAssetOptions } = {} } = useGetOffRampAssetsQuery()

  // Hooks
  const { getSellAssetUrl } = useLib()

  // State
  const [sellAmount, setSellAmount] = React.useState<string>('')
  const [selectedSellAsset, setSelectedSellAsset] = React.useState<BraveWallet.BlockchainToken>()

  // Memos
  const allSellAssetOptions = React.useMemo(() => {
    if (!allAssetOptions) {
      return []
    }
    return filterTokensByNetworks(
      allAssetOptions,
      offRampNetworks
    )
  }, [offRampNetworks, allAssetOptions])

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

  const openSellAssetLink = React.useCallback(
    ({
      sellAddress,
      sellAsset
    }: {
      sellAddress: string
      sellAsset: BraveWallet.BlockchainToken | undefined
    }) => {
      if (!sellAsset || !defaultFiatCurrency) {
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
        amount:
          sellAsset.coin === CoinType.SOL
            ? new Amount(sellAmount)
                .multiplyByDecimals(sellAsset?.decimals ?? 18)
                .toNumber()
                .toString()
            : new Amount(sellAmount)
                .multiplyByDecimals(sellAsset?.decimals ?? 18)
                .toHex(),
        currencyCode: defaultFiatCurrency
      })
        .then((url) => {
          chrome.tabs.create({ url }, () => {
            if (chrome.runtime.lastError) {
              console.error(
                'tabs.create failed: ' + chrome.runtime.lastError.message
              )
            }
          })
        })
        .catch((e) => console.error(e))
    },
    [getSellAssetUrl, defaultFiatCurrency, sellAmount]
  )

  const checkIsAssetSellSupported = React.useCallback((token: BraveWallet.BlockchainToken) => {
    return allSellAssetOptions.some(
      (asset) =>
        (asset.symbol.toLowerCase() === token.symbol.toLowerCase() ||
          asset.contractAddress.toLowerCase() === token.contractAddress.toLowerCase()) &&
        asset.chainId === token.chainId)
  }, [allSellAssetOptions])

  return {
    allSellAssetOptions,
    selectedSellAsset,
    setSelectedSellAsset,
    selectedSellAssetNetwork,
    sellAmount,
    setSellAmount,
    openSellAssetLink,
    checkIsAssetSellSupported
  }
}
