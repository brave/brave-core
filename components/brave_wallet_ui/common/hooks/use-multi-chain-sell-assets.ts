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
import { openTab } from '../../utils/routes-utils'

// Types
import { BraveWallet } from '../../constants/types'

// Hooks
import {
  useGetDefaultFiatCurrencyQuery,
  useGetOffRampNetworksQuery,
  useGetOffRampAssetsQuery,
  useLazyGetSellAssetUrlQuery
} from '../slices/api.slice'

export const useMultiChainSellAssets = () => {
  // Queries
  const { data: offRampNetworks = [] } = useGetOffRampNetworksQuery()
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
  const { data: { allAssetOptions } = {} } = useGetOffRampAssetsQuery()

  // Hooks
  const [getSellAssetUrl] = useLazyGetSellAssetUrlQuery()

  // State
  const [sellAmount, setSellAmount] = React.useState<string>('')
  const [selectedSellAsset, setSelectedSellAsset] =
    React.useState<BraveWallet.BlockchainToken>()

  // Memos
  const allSellAssetOptions = React.useMemo(() => {
    if (!allAssetOptions) {
      return []
    }
    return filterTokensByNetworks(allAssetOptions, offRampNetworks)
  }, [offRampNetworks, allAssetOptions])

  const selectedSellAssetNetwork = React.useMemo(():
    | BraveWallet.NetworkInfo
    | undefined => {
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
    async ({
      sellAddress,
      sellAsset
    }: {
      sellAddress: string
      sellAsset: BraveWallet.BlockchainToken | undefined
    }) => {
      if (!sellAsset || !defaultFiatCurrency) {
        return
      }

      try {
        const url = await getSellAssetUrl({
          assetSymbol: getRampAssetSymbol(sellAsset, true),
          offRampProvider: BraveWallet.OffRampProvider.kRamp,
          chainId: sellAsset.chainId,
          address: sellAddress,
          amount: new Amount(sellAmount)
            .multiplyByDecimals(sellAsset?.decimals ?? 18)
            .toNumber()
            .toString(),
          fiatCurrencyCode: defaultFiatCurrency
        }).unwrap()

        if (url) {
          openTab(url)
        }
      } catch (error) {
        console.error(error)
      }
    },
    [getSellAssetUrl, defaultFiatCurrency, sellAmount]
  )

  const checkIsAssetSellSupported = React.useCallback(
    (token: BraveWallet.BlockchainToken) => {
      return allSellAssetOptions.some(
        (asset) =>
          (asset.symbol.toLowerCase() === token.symbol.toLowerCase() ||
            asset.contractAddress.toLowerCase() ===
              token.contractAddress.toLowerCase()) &&
          asset.chainId === token.chainId
      )
    },
    [allSellAssetOptions]
  )

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
