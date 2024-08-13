// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { useMemo } from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// hooks
import { useSafeWalletSelector } from './use-safe-selector'
import { useGetTokenBalancesForChainIdQuery } from '../slices/api.slice'
import { defaultQuerySubscriptionOptions } from '../slices/constants'

// Types / constants
import { BraveWallet, CoinTypes } from '../../constants/types'
import { WalletSelectors } from '../selectors'

interface Arg {
  network: Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>
  accounts: BraveWallet.AccountInfo[]
  tokens: BraveWallet.BlockchainToken[]
}

const coinTypesMapping = {
  [BraveWallet.CoinType.SOL]: CoinTypes.SOL,
  [BraveWallet.CoinType.ETH]: CoinTypes.ETH,
  [BraveWallet.CoinType.FIL]: CoinTypes.FIL,
  [BraveWallet.CoinType.BTC]: CoinTypes.BTC,
  [BraveWallet.CoinType.ZEC]: CoinTypes.ZEC
}

export const useScopedBalanceUpdater = (arg: Arg | typeof skipToken) => {
  // redux
  const isWalletLocked = useSafeWalletSelector(WalletSelectors.isWalletLocked)
  const isWalletCreated = useSafeWalletSelector(WalletSelectors.isWalletCreated)
  const hasInitialized = useSafeWalletSelector(WalletSelectors.hasInitialized)

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
      hasInitialized &&
      args
      ? args
      : skipToken,
    defaultQuerySubscriptionOptions
  )
}

export default useScopedBalanceUpdater
