// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mapLimit } from 'async'
import { BraveWallet } from '../../../constants/types'
import {
  getAllNetworksList,
  handleEndpointError
} from '../../../utils/api-utils'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

export const dappsEndpoints = ({
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    getTopDapps: query<BraveWallet.Dapp[], void>({
      queryFn: async (_arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { blockchainRegistry } = api
          const networks = await getAllNetworksList(api)

          const dapps: BraveWallet.Dapp[] = (
            await mapLimit(
              networks,
              10,
              async (network: BraveWallet.NetworkInfo) => {
                const { chainId, coin } = network
                const networkDapps = await blockchainRegistry.getTopDapps(
                  chainId,
                  coin
                )
                return networkDapps.dapps
              }
            )
          ).flat(1)

          return {
            data: dapps
          }
        } catch (error) {
          return handleEndpointError(endpoint, 'Error fetching dapps', error)
        }
      },
      providesTags: ['Dapps']
    })
  }
}
