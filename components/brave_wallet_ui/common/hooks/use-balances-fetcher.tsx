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

type Arg = Pick<GetTokenBalancesRegistryArg, 'networks'> & {
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
          useAnkrBalancesFeature
        }
      : skipToken,
    {
      ...querySubscriptionOptions60s,
      selectFromResult: (res) => ({
        data: res.data,
        /**
         * The data is streamed and may be `null` if nothing is persisted yet.
         * res.isLoading is `false` when streaming is in progress
         */
        isLoading: !res.data
      })
    }
  )
}

export default useBalancesFetcher
