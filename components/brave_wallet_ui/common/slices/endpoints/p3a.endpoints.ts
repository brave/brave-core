// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Types
import {
  BraveWallet,
  CoinType,
  SupportedCoinTypes,
  SupportedTestNetworks
} from '../../../constants/types'
import { TokenBalancesRegistry } from '../../../common/slices/entities/token-balance.entity'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// Utils
import { loadTimeData } from '../../../../common/loadTimeData'

export const p3aEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    reportActiveWalletsToP3A: mutation<
      { success: boolean },
      TokenBalancesRegistry
    >({
      queryFn: async (registry, { dispatch }, extraOptions, baseQuery) => {
        try {
          const {
            data: { braveWalletP3A },
            cache
          } = baseQuery(undefined)

          const { accounts } = await cache.getAllAccounts()

          const activeWalletCount: {
            [coin: CoinType]: number
          } = {}

          const countTestNetworks = loadTimeData.getBoolean(
            BraveWallet.P3A_COUNT_TEST_NETWORKS_LOAD_TIME_KEY
          )

          for (const address of Object.keys(registry)) {
            const account = accounts.find(
              (account) => account.accountId.address === address
            )

            if (!account) {
              continue
            }

            const coin = account.accountId.coin

            if (!SupportedCoinTypes.includes(coin)) {
              continue
            }

            if (activeWalletCount[coin] === undefined) {
              activeWalletCount[coin] = 0
            }

            if (
              countTestNetworks &&
              Object.keys(registry[address]).some((chainId) =>
                SupportedTestNetworks.includes(chainId)
              )
            ) {
              activeWalletCount[coin] += 1
              continue
            }

            activeWalletCount[coin] += 1
          }

          for (const [coin, count] of Object.entries(activeWalletCount)) {
            braveWalletP3A.recordActiveWalletCount(count, parseInt(coin))
          }

          return {
            data: { success: true }
          }
        } catch (error) {
          return {
            error: `Unable to record active wallet count: ${error}`
          }
        }
      }
    })
  }
}
