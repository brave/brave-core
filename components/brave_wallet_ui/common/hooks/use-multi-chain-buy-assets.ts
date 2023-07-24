// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// utils
import {
  filterTokensByNetworks,
  getRampAssetSymbol,
  isSelectedAssetInAssetOptions
} from '../../utils/asset-utils'

// types
import {
  BraveWallet,
  BuyOption,
  SupportedTestNetworks
} from '../../constants/types'
import { WalletSelectors } from '../selectors'
import { PageSelectors } from '../../page/selectors'

// options
import { BuyOptions } from '../../options/buy-with-options'

// hooks
import { useIsMounted } from './useIsMounted'
import { useLib } from './useLib'
import {
  useUnsafePageSelector,
  useUnsafeWalletSelector
} from './use-safe-selector'
import {
  useGetNetworkQuery,
  useGetOnRampNetworksQuery,
  useGetSelectedChainQuery
} from '../slices/api.slice'

export const useMultiChainBuyAssets = () => {
  // redux
  const selectedCurrency = useUnsafeWalletSelector(WalletSelectors.selectedCurrency)
  const reduxSelectedAsset = useUnsafePageSelector(PageSelectors.selectedAsset)

  // custom hooks
  const isMounted = useIsMounted()
  const { getAllBuyAssets, getBuyAssetUrl } = useLib()

  // state
  const [buyAmount, setBuyAmount] = React.useState<string>('')
  const [selectedAsset, setSelectedAsset] = React.useState<BraveWallet.BlockchainToken | undefined>()
  const [options, setOptions] = React.useState<
    {
      rampAssetOptions: BraveWallet.BlockchainToken[]
      sardineAssetOptions: BraveWallet.BlockchainToken[]
      transakAssetOptions: BraveWallet.BlockchainToken[]
      stripeAssetOptions: BraveWallet.BlockchainToken[]
      allAssetOptions: BraveWallet.BlockchainToken[]
    }
  >({
    rampAssetOptions: [],
    sardineAssetOptions: [],
    transakAssetOptions: [],
    stripeAssetOptions: [],
    allAssetOptions: []
  })

  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const { data: buyAssetNetworks = [] } = useGetOnRampNetworksQuery()
  const { data: selectedAssetNetwork } = useGetNetworkQuery(
    selectedAsset ?? skipToken
  )

  // memos
  const selectedAssetBuyOptions: BuyOption[] = React.useMemo(() => {
    const { rampAssetOptions, sardineAssetOptions, transakAssetOptions, stripeAssetOptions } = options
    const onRampAssetMap = {
      [BraveWallet.OnRampProvider.kRamp]: rampAssetOptions,
      [BraveWallet.OnRampProvider.kSardine]: sardineAssetOptions,
      [BraveWallet.OnRampProvider.kTransak]: transakAssetOptions,
      [BraveWallet.OnRampProvider.kStripe]: stripeAssetOptions
    }
    return selectedAsset
      ? [...BuyOptions]
        .filter(buyOption => isSelectedAssetInAssetOptions(selectedAsset, onRampAssetMap[buyOption.id]))
        .sort((optionA, optionB) => optionA.name.localeCompare(optionB.name))
      : []
  }, [selectedAsset, options])

  const isSelectedNetworkSupported = React.useMemo(() => {
    if (!selectedNetwork) return false

    // Test networks are not supported in buy tab
    if (SupportedTestNetworks.includes(selectedNetwork.chainId.toLowerCase())) {
      return false
    }

    return options.allAssetOptions
      .map(asset => asset.chainId.toLowerCase())
      .includes(selectedNetwork.chainId.toLowerCase())
  }, [options.allAssetOptions, selectedNetwork])

  const assetsForFilteredNetwork = React.useMemo(() => {
    return options.allAssetOptions.filter(asset => selectedNetwork?.chainId === asset.chainId)
  }, [selectedNetwork, options.allAssetOptions])

  // methods
  const getAllBuyOptionsAllChains = React.useCallback(() => {
    getAllBuyAssets()
      .then(result => {
        if (isMounted && result) {
          setOptions({
            rampAssetOptions: filterTokensByNetworks(result.rampAssetOptions, buyAssetNetworks),
            sardineAssetOptions: filterTokensByNetworks(result.sardineAssetOptions, buyAssetNetworks),
            transakAssetOptions: filterTokensByNetworks(result.transakAssetOptions, buyAssetNetworks),
            stripeAssetOptions: filterTokensByNetworks(result.stripeAssetOptions, buyAssetNetworks),
            allAssetOptions: filterTokensByNetworks(result.allAssetOptions, buyAssetNetworks)
          })
        }
      })
  }, [getAllBuyAssets, buyAssetNetworks, isMounted])

  const openBuyAssetLink = React.useCallback(({ buyOption, depositAddress }: {
    buyOption: BraveWallet.OnRampProvider
    depositAddress: string
  }) => {
    if (!selectedAsset || !selectedAssetNetwork) {
      return
    }

    const asset = {
      ...selectedAsset,
      symbol:
        buyOption === BraveWallet.OnRampProvider.kRamp
          ? getRampAssetSymbol(selectedAsset)
          : buyOption === BraveWallet.OnRampProvider.kStripe
          ? selectedAsset.symbol.toLowerCase()
          : selectedAsset.symbol
    }
    const currencyCode = selectedCurrency
      ? selectedCurrency.currencyCode
      : 'USD'

    getBuyAssetUrl({
      asset,
      onRampProvider: buyOption,
      chainId: selectedAssetNetwork.chainId,
      address: depositAddress,
      amount: buyAmount,
      currencyCode:
        buyOption === BraveWallet.OnRampProvider.kStripe
          ? currencyCode.toLowerCase()
          : currencyCode
    })
      .then((url) => {
        if (chrome.tabs !== undefined) {
          chrome.tabs.create({ url }, () => {
            if (chrome.runtime.lastError) {
              console.error(
                'tabs.create failed: ' + chrome.runtime.lastError.message
              )
            }
          })
        } else {
          // Tabs.create is desktop specific. Using window.open for android.
          window.open(url, "_blank", 'noopener');
        }
      })
      .catch((e) => console.error(e))
  }, [
    selectedAsset,
    selectedAssetNetwork,
    getBuyAssetUrl,
    buyAmount,
    selectedCurrency
  ])

  const isReduxSelectedAssetBuySupported = React.useMemo(() => {
    return options.allAssetOptions.some((asset) => asset.symbol.toLowerCase() === reduxSelectedAsset?.symbol.toLowerCase())
  }, [options.allAssetOptions, reduxSelectedAsset?.symbol])

  return {
    allAssetOptions: options.allAssetOptions,
    rampAssetOptions: options.rampAssetOptions,
    selectedAsset,
    setSelectedAsset,
    selectedAssetNetwork,
    selectedAssetBuyOptions,
    buyAssetNetworks,
    getAllBuyOptionsAllChains,
    buyAmount,
    setBuyAmount,
    openBuyAssetLink,
    isReduxSelectedAssetBuySupported,
    isSelectedNetworkSupported,
    assetsForFilteredNetwork
  }
}
