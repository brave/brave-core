// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import {
  ExternalWalletProvider //
} from '../../../../brave_rewards/resources/shared/lib/external_wallet'
import { BraveWallet } from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// proxies
import {
  BraveRewardsProxy,
  WalletStatus,
  getBraveRewardsProxy
} from '../../async/brave_rewards_api_proxy'

// utils
import { handleEndpointError } from '../../../utils/api-utils'
import {
  getNormalizedExternalRewardsNetwork,
  getNormalizedExternalRewardsWallet,
  getRewardsBATToken,
  getRewardsProviderName
} from '../../../utils/rewards_utils'

/**
 * A function to return the ref to either the main api proxy, or a mocked proxy
 * @returns function that returns an ApiProxy instance
 */
export let rewardsProxyFetcher = getBraveRewardsProxy

/**
 * Assigns a function to use for fetching a BraveRewardsProxy
 * (useful for injecting spies during testing)
 * @param fetcher A function to return the ref to either the main api proxy,
 *  or a mocked proxy
 */
export const setRewardsProxyFetcher = (fetcher: () => BraveRewardsProxy) => {
  rewardsProxyFetcher = fetcher
}
interface BraveRewardsInfo {
  isRewardsEnabled: boolean
  balance: number | undefined
  rewardsToken: BraveWallet.BlockchainToken | undefined
  provider: ExternalWalletProvider | undefined
  providerName: string
  status: WalletStatus
  rewardsAccount: BraveWallet.AccountInfo | undefined
  rewardsNetwork: BraveWallet.NetworkInfo | undefined
  accountLink: string | undefined
}

export const emptyRewardsInfo: BraveRewardsInfo = {
  isRewardsEnabled: false,
  balance: undefined,
  rewardsToken: undefined,
  provider: undefined,
  providerName: '',
  status: WalletStatus.kNotConnected,
  rewardsAccount: undefined,
  rewardsNetwork: undefined,
  accountLink: undefined
} as const

export function braveRewardsApiEndpoints({
  mutation,
  query
}: WalletApiEndpointBuilderParams) {
  return {
    getRewardsInfo: query<BraveRewardsInfo, void>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const isRewardsEnabled =
            await rewardsProxyFetcher().getRewardsEnabled()

          if (!isRewardsEnabled) {
            return {
              data: emptyRewardsInfo
            }
          }

          const balance = await rewardsProxyFetcher().fetchBalance()

          const { provider, status, links } =
            (await rewardsProxyFetcher().getExternalWallet()) || {}

          const rewardsToken = getRewardsBATToken(provider)

          const isConnected = status === WalletStatus.kConnected

          const rewardsAccount = isConnected
            ? getNormalizedExternalRewardsWallet(provider)
            : undefined

          const rewardsNetwork = isConnected
            ? getNormalizedExternalRewardsNetwork(provider)
            : undefined

          return {
            data: {
              isRewardsEnabled: true,
              balance,
              provider,
              rewardsToken,
              status: status || WalletStatus.kNotConnected,
              rewardsAccount,
              rewardsNetwork,
              accountLink: links?.account,
              providerName: getRewardsProviderName(provider)
            }
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to get Brave Rewards information',
            error
          )
        }
      },
      providesTags: ['BraveRewards-Info']
    })
  } as const
}
