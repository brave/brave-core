// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { useMemo } from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// hooks
import {
  useGetTokenBalancesForChainIdQuery,
  useGetWalletInfoQuery
} from '../slices/api.slice'
import { defaultQuerySubscriptionOptions } from '../slices/constants'

// Types / constants
import { BraveWallet, CoinTypes } from '../../constants/types'

interface Arg {
  network: Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>
  accounts: BraveWallet.AccountInfo[]
  tokens?: BraveWallet.BlockchainToken[]
}

const coinTypesMapping = {
  [BraveWallet.CoinType.SOL]: CoinTypes.SOL,
  [BraveWallet.CoinType.ETH]: CoinTypes.ETH,
  [BraveWallet.CoinType.FIL]: CoinTypes.FIL,
  [BraveWallet.CoinType.BTC]: CoinTypes.BTC,
  [BraveWallet.CoinType.ZEC]: CoinTypes.ZEC
}

export const useScopedBalanceUpdater = (arg: Arg | typeof skipToken) => {
  // queries
  const { data: walletInfo } = useGetWalletInfoQuery()
  const isWalletLocked = walletInfo?.isWalletLocked ?? true
  const isWalletCreated = walletInfo?.isWalletCreated ?? false

  const args = useMemo(() => {
    if (arg === skipToken || !arg.accounts) {
      return []
    }

    const nonSolArgs = arg.accounts.flatMap((account) =>
      account.accountId.coin !== CoinTypes.SOL &&
      arg.tokens &&
      coinTypesMapping[account.accountId.coin] !== undefined
        ? [
            {
              accountId: account.accountId,
              chainId: arg.network.chainId,
              coin: coinTypesMapping[account.accountId.coin],
              tokens: arg.tokens
            }
          ]
        : []
    )

    const solArgs = arg.accounts.flatMap((account) =>
      account.accountId.coin === CoinTypes.SOL
        ? [
            {
              accountId: account.accountId,
              chainId: arg.network.chainId,
              coin: CoinTypes.SOL,
              tokens: arg.tokens
            }
          ]
        : []
    )

    return [...nonSolArgs, ...solArgs]
  }, [arg])

  return useGetTokenBalancesForChainIdQuery(
    arg !== skipToken &&
      // account and network CoinType can be inconsistent during intermittent
      // updates.
      arg.accounts.every(
        (account) => account.accountId.coin === arg.network.coin
      ) &&
      !isWalletLocked &&
      isWalletCreated &&
      args
      ? args
      : skipToken,
    defaultQuerySubscriptionOptions
  )
}

export default useScopedBalanceUpdater
