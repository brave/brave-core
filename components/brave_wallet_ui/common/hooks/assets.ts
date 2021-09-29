// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  AccountAssetOptionType,
  TokenInfo,
  WalletAccountType
} from '../../constants/types'
import { BAT, ETH } from '../../options/asset-options'

export default function useAssets (
  selectedAccount: WalletAccountType,
  fullTokenList: TokenInfo[],
  userVisibleTokensInfo: TokenInfo[]
) {
  const tokenOptions: TokenInfo[] = React.useMemo(
    () =>
      fullTokenList.map((token) => ({
        ...token,
        logo: `chrome://erc-token-images/${token.logo}`
      })),
    [fullTokenList]
  )

  const assetOptions: AccountAssetOptionType[] = React.useMemo(() => {
    const tokens = tokenOptions
      .filter((token) => token.symbol !== 'BAT')
      .map((token) => ({
        asset: token,
        assetBalance: '0',
        fiatBalance: '0'
      }))

    return [ETH, BAT, ...tokens]
  }, [tokenOptions])

  const userVisibleTokenOptions: TokenInfo[] = React.useMemo(
    () =>
      userVisibleTokensInfo.map((token) => ({
        ...token,
        logo: `chrome://erc-token-images/${token.logo}`
      })),
    [userVisibleTokensInfo]
  )

  const sendAssetOptions = selectedAccount?.tokens?.map((token) => ({
    ...token,
    asset: {
      ...token.asset,
      logo:
        token.asset.symbol === 'ETH'
          ? ETH.asset.logo
          : `chrome://erc-token-images/${token.asset.logo}`
    }
  }))

  return {
    tokenOptions,
    assetOptions,
    userVisibleTokenOptions,
    sendAssetOptions
  }
}
