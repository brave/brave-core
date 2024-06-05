// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { EntityId } from '@reduxjs/toolkit'
import { flatMapLimit } from 'async'

// types
import { BraveWallet } from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// utils
import { handleEndpointError } from '../../../utils/api-utils'
import { getEntitiesListFromEntityState } from '../../../utils/entities.utils'

export const dappRadarEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    getTopDapps: query<BraveWallet.Dapp[], EntityId[] | undefined>({
      queryFn: async (chainIdsArg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api, cache } = baseQuery(undefined)
          const netsRegistry = await cache.getNetworksRegistry()

          const chainIds =
            chainIdsArg && chainIdsArg.length > 0
              ? chainIdsArg
              : netsRegistry.visibleIds

          const networks = getEntitiesListFromEntityState(
            netsRegistry,
            chainIds
          )

          if (!networks.length) {
            throw new Error('No chains to fetch for ids: ' + chainIds.join())
          }

          // using only visible networks to speedup load-times
          const topDapps: BraveWallet.Dapp[] = await flatMapLimit(
            networks,
            10,
            async (net: BraveWallet.NetworkInfo) => {
              const { dapps } = await api.blockchainRegistry.getTopDapps(
                net.chainId,
                net.coin
              )
              return dapps
            }
          )

          const uniqueTopDapps: BraveWallet.Dapp[] = []

          for (const dapp of topDapps) {
            if (!uniqueTopDapps.some((d) => d.id === dapp.id)) {
              uniqueTopDapps.push(dapp)
            }
          }

          return {
            data: uniqueTopDapps
          }
        } catch (error) {
          return handleEndpointError(endpoint, 'unable to get top dApps', error)
        }
      }
    })
  }
}
