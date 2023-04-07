// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

// Constants
import {
  WalletState
} from '../../constants/types'

// Hooks
import usePricing from './pricing'

// utils
import { getBalance } from '../../utils/balance-utils'
import { useGetSelectedChainQuery } from '../slices/api.slice'

export function useAssets () {
  // redux
  const {
    selectedAccount,
    userVisibleTokensInfo,
    transactionSpotPrices: spotPrices
  } = useSelector((state: { wallet: WalletState }) => state.wallet)

  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()

  // custom hooks
  const { computeFiatAmount } = usePricing(spotPrices)

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

      const bFiatBalance = computeFiatAmount(bBalance, b.symbol, b.decimals, b.contractAddress, b.chainId)
      const aFiatBalance = computeFiatAmount(aBalance, a.symbol, a.decimals, a.contractAddress, a.chainId)

      return bFiatBalance.toNumber() - aFiatBalance.toNumber()
    })
  }, [selectedAccount, assetsByNetwork, getBalance, computeFiatAmount])

  return {
    sendAssetOptions: assetsByNetwork,
    panelUserAssetList: assetsByValueAndNetwork
  }
}

export default useAssets
