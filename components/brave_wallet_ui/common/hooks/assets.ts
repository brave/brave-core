// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Constants
import {
  WalletState
} from '../../constants/types'

// utils
import { getBalance } from '../../utils/balance-utils'
import Amount from '../../utils/amount'
import { getPriceIdForToken } from '../../utils/api-utils'
import { computeFiatAmount } from '../../utils/pricing-utils'

import {
  useGetSelectedChainQuery,
  useGetTokenSpotPricesQuery
} from '../slices/api.slice'
import { querySubscriptionOptions60s } from '../slices/constants'

export function useAssets () {
  // redux
  const {
    selectedAccount,
    userVisibleTokensInfo,
  } = useSelector((state: { wallet: WalletState }) => state.wallet)

  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()

  // memos
  const assetsByNetwork = React.useMemo(() => {
    if (!userVisibleTokensInfo || !selectedNetwork) {
      return []
    }
    // We also filter by coinType here because localhost
    // networks share the same chainId.
    return userVisibleTokensInfo.filter((token) =>
      token.chainId === selectedNetwork.chainId &&
      token.coin === selectedNetwork.coin
    )
  }, [userVisibleTokensInfo, selectedNetwork])

  const tokenPriceIds = React.useMemo(
    () =>
      assetsByNetwork
        .filter(token => new Amount(getBalance(selectedAccount, token)).gt(0))
        .filter(token => !token.isErc721 && !token.isErc1155 && !token.isNft)
        .map(getPriceIdForToken),
    []
  )

  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    tokenPriceIds ? { ids: tokenPriceIds } : skipToken,
    querySubscriptionOptions60s
  )

  const assetsByValueAndNetwork = React.useMemo(() => {
    if (!assetsByNetwork?.length) {
      return []
    }

    if (!selectedAccount) {
      return []
    }

    return assetsByNetwork.sort(function (a, b) {
      const aBalance = getBalance(selectedAccount, a)
      const bBalance = getBalance(selectedAccount, b)

      const bFiatBalance = computeFiatAmount({
        spotPriceRegistry,
        value: bBalance,
        token: b
      })
      const aFiatBalance = computeFiatAmount({
        spotPriceRegistry,
        value: aBalance,
        token: a
      })

      return bFiatBalance.minus(aFiatBalance).toNumber()
    })
  }, [selectedAccount, assetsByNetwork, getBalance, spotPriceRegistry])

  return {
    sendAssetOptions: assetsByNetwork,
    panelUserAssetList: assetsByValueAndNetwork
  }
}

export default useAssets
