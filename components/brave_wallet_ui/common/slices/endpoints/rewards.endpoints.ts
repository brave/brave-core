// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import { WalletApiEndpointBuilderParams } from '../api-base.slice'
import { BraveRewardsInfo } from '../../../constants/types'

// utils
import { handleEndpointError } from '../../../utils/api-utils'

export function braveRewardsApiEndpoints({
  mutation,
  query
}: WalletApiEndpointBuilderParams) {
  return {
    getRewardsInfo: query<BraveRewardsInfo, void>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { cache } = baseQuery(undefined)
          return {
            data: cache.rewardsInfo || (await cache.getBraveRewardsInfo())
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
