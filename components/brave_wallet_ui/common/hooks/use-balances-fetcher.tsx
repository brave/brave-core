// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { skipToken } from '@reduxjs/toolkit/query/react'

// hooks
import { useSafeWalletSelector } from './use-safe-selector'
import { useGetTokenBalancesRegistryQuery } from '../slices/api.slice'
import { querySubscriptionOptions60s } from '../slices/constants'

// Types
import { BraveWallet } from '../../constants/types'
import { WalletSelectors } from '../selectors'
import {
  GetTokenBalancesRegistryArg //
} from '../slices/endpoints/token_balances.endpoints'

// utils
import {
  getPersistedTokenBalancesSubset //
} from '../../utils/local-storage-utils'

type Arg = Pick<GetTokenBalancesRegistryArg, 'networks' | 'isSpamRegistry'> & {
  accounts: BraveWallet.AccountInfo[]
}

export const useBalancesFetcher = (arg: Arg | typeof skipToken) => {
  // redux
  const isWalletLocked = useSafeWalletSelector(WalletSelectors.isWalletLocked)
  const isWalletCreated = useSafeWalletSelector(WalletSelectors.isWalletCreated)
  const hasInitialized = useSafeWalletSelector(WalletSelectors.hasInitialized)
  const useAnkrBalancesFeature = useSafeWalletSelector(
    WalletSelectors.isAnkrBalancesFeatureEnabled
  )

  return useGetTokenBalancesRegistryQuery(
    arg !== skipToken &&
      !isWalletLocked &&
      isWalletCreated &&
      hasInitialized &&
      arg.accounts.length &&
      arg.networks.length
      ? {
          accountIds: arg.accounts.map((account) => account.accountId),
          networks: arg.networks.map(
            ({ chainId, coin, supportedKeyrings }) => ({
              chainId,
              coin,
              supportedKeyrings
            })
          ),
          useAnkrBalancesFeature,
          isSpamRegistry: arg.isSpamRegistry
        }
      : skipToken,
    {
      ...querySubscriptionOptions60s,
      selectFromResult: (res) => ({
        data:
          res.data ??
          (arg === skipToken
            ? null
            : getPersistedTokenBalancesSubset({
                accountIds: arg.accounts.map((account) => account.accountId),
                networks: arg.networks,
                isSpamRegistry: arg.isSpamRegistry
              })),
        isLoading: res.isLoading
      })
    }
  )
}

export default useBalancesFetcher
