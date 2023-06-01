// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { skipToken } from '@reduxjs/toolkit/query/react'

// hooks
import { useSafeWalletSelector } from './use-safe-selector'
import { useGetTokenBalancesForChainIdQuery } from '../slices/api.slice'

// Types / constants
import { BraveWallet, CoinTypes } from '../../constants/types'
import { WalletSelectors } from '../selectors'

interface Arg {
  network: BraveWallet.NetworkInfo
  account: BraveWallet.AccountInfo
  tokens?: BraveWallet.BlockchainToken[]
}

export const useScopedBalanceUpdater = (arg: Arg | typeof skipToken) => {
  // redux
  const isWalletLocked = useSafeWalletSelector(WalletSelectors.isWalletLocked)
  const isWalletCreated = useSafeWalletSelector(WalletSelectors.isWalletCreated)
  const hasInitialized = useSafeWalletSelector(WalletSelectors.hasInitialized)

  return useGetTokenBalancesForChainIdQuery(
    arg !== skipToken &&
      arg.account &&
      arg.network &&
      // account and network CoinType can be inconsistent during intermittent
      // updates.
      arg.account.accountId.coin === arg.network.coin &&
      !isWalletLocked &&
      isWalletCreated &&
      hasInitialized
      ? arg.account.accountId.coin === CoinTypes.SOL
        ? {
            chainId: arg.network.chainId,
            coin: CoinTypes.SOL,
            pubkey: arg.account.address,
            tokens: arg.tokens
          }
        : arg.account.accountId.coin === CoinTypes.ETH && arg.tokens
        ? {
            chainId: arg.network.chainId,
            coin: CoinTypes.ETH,
            address: arg.account.address,
            tokens: arg.tokens
          }
        : skipToken
      : skipToken,
    {
      refetchOnFocus: true,
      pollingInterval: 15000,
      refetchOnMountOrArgChange: 15000,
      refetchOnReconnect: true
    }
  )
}

export default useScopedBalanceUpdater
