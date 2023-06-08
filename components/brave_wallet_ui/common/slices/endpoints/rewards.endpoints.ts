// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// proxies
import {
  BraveRewardsProxy,
  RewardsExternalWallet,
  getBraveRewardsProxy
} from '../../async/brave_rewards_api_proxy'

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

export function braveRewardsApiEndpoints({
  mutation,
  query
}: WalletApiEndpointBuilderParams) {
  return {
    getRewardsEnabled: query<boolean, void>({
      queryFn: async (arg, api, extraOptions, baseQuery) => {
        try {
          const enabled = await rewardsProxyFetcher().getRewardsEnabled()
          return { data: enabled }
        } catch (error) {
          const message = `Failed to check if rewards are enabled: ${
            error.toString() //
          }`
          console.error(message)
          return {
            error: message
          }
        }
      },
      providesTags: ['BraveRewards-Enabled']
    }),

    getRewardsBalance: query<number, void>({
      queryFn: async (arg, api, extraOptions, baseQuery) => {
        try {
          const balance = await rewardsProxyFetcher().fetchBalance()
          return { data: balance || 0 }
        } catch (error) {
          const message = `Failed to fetch rewards balance: ${
            error.toString() //
          }`
          console.error(message)
          return {
            error: message
          }
        }
      },
      providesTags: ['BraveRewards-RewardsBalance']
    }),

    getExternalRewardsWallet: query<RewardsExternalWallet | null, void>({
      queryFn: async (arg, api, extraOptions, baseQuery) => {
        try {
          const externalWallet = await rewardsProxyFetcher().getExternalWallet()
          return { data: externalWallet }
        } catch (error) {
          const message = `Failed to fetch rewards balance: ${
            error.toString() //
          }`
          console.error(message)
          return {
            error: message
          }
        }
      },
      providesTags: ['BraveRewards-ExternalWallet']
    })
  } as const
}
