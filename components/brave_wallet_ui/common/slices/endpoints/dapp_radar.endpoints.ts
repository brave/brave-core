// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { flatMapLimit } from 'async'

// types
import { BraveWallet } from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// utils
import { handleEndpointError } from '../../../utils/api-utils'
import { getEntitiesListFromEntityState } from '../../../utils/entities.utils'
import {
  dappRadarChainNamesToChainIdMapping //
} from '../../constants/dapp_radar'

export const dappRadarEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    getTopDapps: query<BraveWallet.Dapp[], void>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api, cache } = baseQuery(undefined)
          const netsRegistry = await cache.getNetworksRegistry()

          const chainIds = Object.values(
            dappRadarChainNamesToChainIdMapping
          ).filter((chainId) => !!chainId)

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

          // swap chain names with chainIds
          const parsedUniqueTopDapps: BraveWallet.Dapp[] = uniqueTopDapps.map(
            (dapp) => {
              return {
                ...dapp,
                chains: dapp.chains
                  .filter((chain) => {
                    return !!dappRadarChainNamesToChainIdMapping[chain]
                  })
                  .map((chain) => {
                    return dappRadarChainNamesToChainIdMapping[chain]
                  })
              }
            }
          )

          return {
            // filter by network ids args until core supports more chains
            data: parsedUniqueTopDapps
          }
        } catch (error) {
          return handleEndpointError(endpoint, 'unable to get top dApps', error)
        }
      }
    })
  }
}
