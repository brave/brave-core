// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// hooks
import { NetworkInfo } from '@brave/swap-interface'
import * as React from 'react'

// types
import { BraveWallet } from '../../constants/types'
import { makeNetworkInfo } from '../../page/screens/swap/adapters'
import { getEntitiesListFromEntityState } from '../../utils/entities.utils'

// hooks
import {
  useGetAllNetworksQuery,
  useGetSelectedChainQuery,
  useGetSwapSupportedChainIdsQuery
} from '../slices/api.slice'

// selectors
import {
  networkEntityAdapter,
  emptyNetworksRegistry,
  selectAllNetworksFromQueryResult,
  selectOffRampNetworksFromQueryResult
} from '../slices/entities/network.entity'

const emptyNetworks: BraveWallet.NetworkInfo[] = []
const emptyIds: string[] = []

export const useNetworkQuery = (
  {
    chainId,
    coin
  }: {
    chainId: string
    coin: BraveWallet.CoinType
  },
  opts?: { skip?: boolean }
) => {
  return useGetAllNetworksQuery(undefined, {
    selectFromResult: (res) => ({
      ...res,
      network: res.data
        ? res.data.entities[
            networkEntityAdapter.selectId({
              chainId,
              coin
            })
          ]
        : undefined
    }),
    skip: opts?.skip
  })
}

export const useNetwork = (
  arg:
    | {
        chainId: string
        coin: BraveWallet.CoinType
      }
    | undefined,
  opts?: { skip?: boolean }
) => {
  const { data: registry = emptyNetworksRegistry } = useGetAllNetworksQuery(
    undefined,
    { skip: opts?.skip }
  )

  return arg?.chainId !== undefined && arg?.coin !== undefined
    ? registry.entities[networkEntityAdapter.selectId(arg)]
    : undefined
}

export const useNetworksListQuery = (
  opts?: { skip?: boolean }
) => {
  const queryResults = useGetAllNetworksQuery(
    undefined,
    {
      selectFromResult: res => ({
        isLoading: res.isLoading,
        networks: selectAllNetworksFromQueryResult(res),
      }),
      skip: opts?.skip
    }
  )

  return queryResults
}

export const useSelectedCoinQuery = (
  opts?: { skip?: boolean }
) => {
  const queryResults = useGetSelectedChainQuery(undefined, {
    selectFromResult: (res) => ({ selectedCoin: res.data?.coin }),
    skip: opts?.skip
  })
  return queryResults
}

export const useOffRampNetworksQuery = (
  opts?: { skip?: boolean }
) => {
  const queryResults = useGetAllNetworksQuery(
    undefined,
    {
      selectFromResult: res => ({
        isLoading: res.isLoading,
        offRampNetworks: selectOffRampNetworksFromQueryResult(res),
      }),
      skip: opts?.skip
    }
  )

  return queryResults
}

export const useSwapSupportedNetworksQuery = () => {
  // queries
  const { data: networksRegistry } =
    useGetAllNetworksQuery()
  const { data: swapSupportedChainIds = emptyIds } =
    useGetSwapSupportedChainIdsQuery(undefined, { skip: !networksRegistry })

  // memos
  const supportedNetworks = React.useMemo(() => {
    if (!networksRegistry) {
      return emptyNetworks
    }

    return getEntitiesListFromEntityState(
      networksRegistry,
      swapSupportedChainIds
    )
  }, [networksRegistry, swapSupportedChainIds])

  return supportedNetworks
}

export const useSwapSupportedNetworkInfos = () => {
  // queries
  const networks = useSwapSupportedNetworksQuery()

  // memos
  const supportedNetInfos: NetworkInfo[] = React.useMemo(() => {
    return networks.map(makeNetworkInfo)
  }, [networks])

  return supportedNetInfos
}
