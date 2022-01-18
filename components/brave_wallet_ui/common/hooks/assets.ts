// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  BraveWallet,
  WalletAccountType
} from '../../constants/types'
import { BAT, ETH } from '../../options/asset-options'

// Hooks
import usePricing from './pricing'
import useBalance from './balance'

export default function useAssets (
  accounts: WalletAccountType[],
  selectedAccount: WalletAccountType,
  selectedNetwork: BraveWallet.EthereumChain,
  fullTokenList: BraveWallet.BlockchainToken[],
  userVisibleTokensInfo: BraveWallet.BlockchainToken[],
  spotPrices: BraveWallet.AssetPrice[],
  getBuyAssets: () => Promise<BraveWallet.BlockchainToken[]>
) {
  const { computeFiatAmount } = usePricing(spotPrices)
  const getBalance = useBalance(selectedNetwork)

  const swapAssetOptions: BraveWallet.BlockchainToken[] = React.useMemo(() => {
    return [
      ETH,

      ...fullTokenList.filter((asset) => asset.symbol === 'BAT'),

      ...userVisibleTokensInfo
        .filter(asset => !['BAT', 'ETH'].includes(asset.symbol)),

      ...fullTokenList
        .filter(asset => !['BAT', 'ETH'].includes(asset.symbol))
        .filter(asset => !userVisibleTokensInfo.some(token => token.symbol === asset.symbol))
    ]
  }, [fullTokenList, userVisibleTokensInfo])

  const [buyAssetOptions, setBuyAssetOptions] = React.useState<BraveWallet.BlockchainToken[]>([BAT, ETH])

  React.useEffect(() => {
    getBuyAssets().then(tokens => {
      setBuyAssetOptions(tokens.map(token => ({
        ...token,
        logo: `chrome://erc-token-images/${token.logo}`
      }) as BraveWallet.BlockchainToken))
    }).catch(e => console.error(e))
  }, [])

  const panelUserAssetList = React.useMemo(() => {
    if (!userVisibleTokensInfo) {
      return []
    }

    if (!selectedAccount) {
      return []
    }

    const nonZeroBalanceAssets = userVisibleTokensInfo.filter(token => {
      const balance = getBalance(selectedAccount, token)
      return balance !== '' && parseInt(balance) > 0
    })

    return nonZeroBalanceAssets.sort(function (a, b) {
      const aBalance = getBalance(selectedAccount, a)
      const bBalance = getBalance(selectedAccount, b)

      const bFiatBalance = computeFiatAmount(bBalance, b.symbol, b.decimals)
      const aFiatBalance = computeFiatAmount(aBalance, a.symbol, a.decimals)

      return Number(bFiatBalance) - Number(aFiatBalance)
    })
  }, [selectedAccount, userVisibleTokensInfo, getBalance, computeFiatAmount])

  return {
    swapAssetOptions,
    sendAssetOptions: userVisibleTokensInfo,
    buyAssetOptions,
    panelUserAssetList
  }
}
