// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

// utils
import { getNetworkInfo } from '../../utils/network-utils'
import {
  getRampAssetSymbol,
  getWyreAssetSymbol,
  isSelectedAssetInAssetOptions
} from '../../utils/asset-utils'

// types
import { BraveWallet, SupportedOnRampNetworks, WalletState } from '../../constants/types'

// Hooks
import { useIsMounted } from './useIsMounted'
import { useLib } from './useLib'
import { BuyOptions } from '../../options/buy-with-options'

export function useMultiChainBuyAssets () {
  // redux
  const networkList = useSelector(({ wallet }: { wallet: WalletState }) => wallet.networkList)
  const selectedCurrency = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedCurrency)

  // custom hooks
  const isMounted = useIsMounted()
  const { getAllBuyAssets, getBuyAssetUrl } = useLib()

  // state
  const [buyAmount, setBuyAmount] = React.useState('')
  const [selectedAsset, setSelectedAsset] = React.useState<BraveWallet.BlockchainToken | undefined>()
  const [options, setOptions] = React.useState<
    {
      wyreAssetOptions: BraveWallet.BlockchainToken[]
      rampAssetOptions: BraveWallet.BlockchainToken[]
      allAssetOptions: BraveWallet.BlockchainToken[]
    }
  >({
    wyreAssetOptions: [],
    rampAssetOptions: [],
    allAssetOptions: []
  })

  // memos
  const buyAssetNetworks = React.useMemo(() => {
    return networkList.filter(n =>
      SupportedOnRampNetworks.includes(n.chainId)
    )
  }, [networkList])

  const selectedAssetNetwork = React.useMemo(() => {
    return selectedAsset ? getNetworkInfo(selectedAsset.chainId, selectedAsset.coin, buyAssetNetworks) : undefined
  }, [selectedAsset, buyAssetNetworks])

  const selectedAssetBuyOptions = React.useMemo(() => {
    return selectedAsset
      ? BuyOptions.filter(buyOption => {
          return isSelectedAssetInAssetOptions(
            selectedAsset,
            buyOption.id === BraveWallet.OnRampProvider.kWyre
              ? options.wyreAssetOptions
              : buyOption.id === BraveWallet.OnRampProvider.kRamp ? options.rampAssetOptions : []
          )
        })
      : []
  }, [selectedAsset, options])

  // methods
  const getAllBuyOptionsAllChains = React.useCallback(() => {
    getAllBuyAssets()
      .then(result => {
        if (isMounted && result) {
          setOptions(result)
        }
      })
  }, [getAllBuyAssets, isMounted])

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
        buyOption === BraveWallet.OnRampProvider.kRamp ? getRampAssetSymbol(selectedAsset)
        : buyOption === BraveWallet.OnRampProvider.kWyre ? getWyreAssetSymbol(selectedAsset)
        : selectedAsset.symbol
    }

    getBuyAssetUrl({
      asset,
      onRampProvider: buyOption,
      chainId: selectedAssetNetwork.chainId,
      address: depositAddress,
      amount: buyAmount,
      currencyCode: selectedCurrency ? selectedCurrency.currencyCode : 'USD'
    })
      .then(url => {
        chrome.tabs.create({ url }, () => {
          if (chrome.runtime.lastError) {
            console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
          }
        })
      })
      .catch(e => console.error(e))
  }, [getBuyAssetUrl, selectedAssetNetwork, buyAmount, selectedAsset])

  return {
    allAssetOptions: options.allAssetOptions,
    rampAssetOptions: options.rampAssetOptions,
    wyreAssetOptions: options.wyreAssetOptions,
    selectedAsset,
    setSelectedAsset,
    selectedAssetNetwork,
    selectedAssetBuyOptions,
    buyAssetNetworks,
    getAllBuyOptionsAllChains,
    buyAmount,
    setBuyAmount,
    openBuyAssetLink
  }
}
