// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  AccountAssetOptionType,
  ERCToken,
  WalletAccountType
} from '../../constants/types'
import { ETH } from '../../options/asset-options'

export default function useAssets (
  selectedAccount: WalletAccountType,
  fullTokenList: ERCToken[],
  userVisibleTokensInfo: ERCToken[]
) {
  const tokenOptions: ERCToken[] = React.useMemo(
    () =>
      fullTokenList.map((token) => ({
        ...token,
        logo: `chrome://erc-token-images/${token.logo}`
      })),
    [fullTokenList]
  )

  const userVisibleTokenOptions: ERCToken[] = React.useMemo(
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
          assetBalance: '0',
          fiatBalance: '0'
        })),
    [userVisibleTokenOptions]
  )

  const assetOptions: AccountAssetOptionType[] = React.useMemo(() => {
    const assets = tokenOptions
      .map((token) => ({
        asset: token,
        assetBalance: '0',
        fiatBalance: '0'
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

  return {
    tokenOptions,
    assetOptions,
    userVisibleTokenOptions,
    sendAssetOptions
  }
}
