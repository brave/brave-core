// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Types
import { BraveWallet, SupportedTestNetworks } from '../../../constants/types'
import { TokenBalancesRegistry } from '../../../common/slices/entities/token-balance.entity'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// Utils
import { loadTimeData } from '../../../../common/loadTimeData'
import { handleEndpointError } from '../../../utils/api-utils'
import { getAccountBalancesKey } from '../../../utils/balance-utils'

export const p3aEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    reportActiveWalletsToP3A: mutation<
      { success: boolean },
      TokenBalancesRegistry
    >({
      queryFn: async (
        tokenBalancesRegistry,
        { endpoint },
        extraOptions,
        baseQuery
      ) => {
        try {
          const {
            data: { braveWalletP3A },
            cache
          } = baseQuery(undefined)

          const { accounts } = await cache.getAllAccounts()

          const activeWalletCount: Record<BraveWallet.CoinType, number> = {}

          const countTestNetworks = loadTimeData.getBoolean(
            BraveWallet.P3A_COUNT_TEST_NETWORKS_LOAD_TIME_KEY
          )

          accounts.map((account) => {
            const accountBalances =
              tokenBalancesRegistry.accounts[
                getAccountBalancesKey(account.accountId)
              ]
            // Assume that if there is an entry for account then that account
            // has some balance.
            if (!accountBalances) {
              return
            }

            const coin = account.accountId.coin

            if (activeWalletCount[coin] === undefined) {
              activeWalletCount[coin] = 0
            }

            const chainIds = Object.keys(accountBalances.chains)

            const hasMainnetBalance = chainIds.some(
              (chainId) => !SupportedTestNetworks.includes(chainId)
            )
            const hasTestnetBalance = chainIds.some((chainId) =>
              SupportedTestNetworks.includes(chainId)
            )

            if (hasMainnetBalance || (countTestNetworks && hasTestnetBalance)) {
              activeWalletCount[coin] += 1
            }
          })

          for (const [coin, count] of Object.entries(activeWalletCount)) {
            braveWalletP3A.recordActiveWalletCount(count, parseInt(coin))
          }

          return {
            data: { success: true }
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Unable to record active wallet count',
            error
          )
        }
      }
    }),

    reportOnboardingAction: mutation<true, BraveWallet.OnboardingAction>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          api.braveWalletP3A.reportOnboardingAction(arg)
          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Unable to report onboarding action',
            error
          )
        }
      }
    })
  }
}
