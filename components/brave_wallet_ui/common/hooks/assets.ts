// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  AccountAssetOptionType,
  BraveWallet,
  WalletAccountType
} from '../../constants/types'
import { BAT, ETH } from '../../options/asset-options'

// Hooks
import usePricing from './pricing'

export default function useAssets (
  accounts: WalletAccountType[],
  selectedAccount: WalletAccountType,
  fullTokenList: BraveWallet.BlockchainToken[],
  userVisibleTokensInfo: BraveWallet.BlockchainToken[],
  spotPrices: BraveWallet.AssetPrice[],
  getBuyAssets: () => Promise<BraveWallet.BlockchainToken[]>
) {
  const { computeFiatAmount } = usePricing(spotPrices)

  const tokenOptions: BraveWallet.BlockchainToken[] = React.useMemo(
    () =>
      fullTokenList.map((token) => ({
        ...token,
        logo: `chrome://erc-token-images/${token.logo}`
      })),
    [fullTokenList]
  )

  const userVisibleTokenOptions: BraveWallet.BlockchainToken[] = React.useMemo(
    () =>
      userVisibleTokensInfo.map((token) => ({
        ...token,
        logo: `chrome://erc-token-images/${token.logo}`
      })),
    [userVisibleTokensInfo]
  )

  const sendAssetOptions: AccountAssetOptionType[] = React.useMemo(
    () =>
      userVisibleTokenOptions
        .map((token) => ({
          asset: token,
          assetBalance: '0'
        })),
    [userVisibleTokenOptions]
  )

  const assetOptions: AccountAssetOptionType[] = React.useMemo(() => {
    const assets = tokenOptions
      .map((token) => ({
        asset: token,
        assetBalance: '0'
      }))

    return [
      ETH,

      ...assets.filter((asset) => asset.asset.symbol === 'BAT'),

      ...sendAssetOptions
        .filter(asset => !['BAT', 'ETH'].includes(asset.asset.symbol)),

      ...assets
        .filter(asset => !['BAT', 'ETH'].includes(asset.asset.symbol))
        .filter(asset => !sendAssetOptions.some(token => token.asset.symbol === asset.asset.symbol))
    ]
  }, [tokenOptions, sendAssetOptions])

  const [buyAssetOptions, setBuyAssetOptions] = React.useState<AccountAssetOptionType[]>([BAT, ETH])

  React.useEffect(() => {
    getBuyAssets().then(tokens => {
      setBuyAssetOptions(tokens.map(token => ({
        asset: {
          ...token,
          logo: `chrome://erc-token-images/${token.logo}`
        },
        assetBalance: '0'
      }) as AccountAssetOptionType))
    }).catch(e => console.error(e))
  }, [])

  const panelUserAssetList = React.useMemo((): AccountAssetOptionType[] => {
    // selectedAccount.tokens can be undefined
    if (!selectedAccount?.tokens) {
      return []
    }

    const formattedList = selectedAccount?.tokens?.sort(function (a, b) {
      const bFiatBalance = computeFiatAmount(b.assetBalance, b.asset.symbol, b.asset.decimals)
      const aFiatBalance = computeFiatAmount(a.assetBalance, a.asset.symbol, a.asset.decimals)
      return Number(bFiatBalance) - Number(aFiatBalance)
    }) // Sorting by Fiat Value

    // Do not show an asset unless the selectedAccount has a balance
    return formattedList.filter((token) => parseFloat(token.assetBalance) !== 0)
    // Using accounts as a dependency here to trigger balance changes
  }, [selectedAccount, accounts])

  return {
    tokenOptions,
    assetOptions,
    userVisibleTokenOptions,
    sendAssetOptions,
    buyAssetOptions,
    panelUserAssetList
  }
}
