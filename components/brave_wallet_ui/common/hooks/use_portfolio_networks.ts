// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// constants
import { emptyRewardsInfo } from '../async/base-query-cache'
import { WalletStatus } from '../../constants/types'
import { LOCAL_STORAGE_KEYS } from '../constants/local-storage-keys'

// hooks
import {
  useGetRewardsInfoQuery,
  useGetVisibleNetworksQuery
} from '../slices/api.slice'
import { useLocalStorage } from './use_local_storage'

// utils
import { networkEntityAdapter } from '../slices/entities/network.entity'
import {
  makeInitialFilteredOutNetworkKeys //
} from '../../utils/local-storage-utils'

export const usePortfolioVisibleNetworks = () => {
  // local-storage
  const [filteredOutPortfolioNetworkKeys] = useLocalStorage(
    LOCAL_STORAGE_KEYS.FILTERED_OUT_PORTFOLIO_NETWORK_KEYS,
    makeInitialFilteredOutNetworkKeys
  )

  // queries
  const { data: networks } = useGetVisibleNetworksQuery()

  const {
    data: {
      status: rewardsStatus,
      rewardsNetwork: externalRewardsNetwork
    } = emptyRewardsInfo
  } = useGetRewardsInfoQuery()

  // Computed
  const displayRewardsInPortfolio = rewardsStatus === WalletStatus.kConnected

  // Memos
  const networksList = React.useMemo(() => {
    return displayRewardsInPortfolio && externalRewardsNetwork
      ? [externalRewardsNetwork].concat(networks)
      : networks
  }, [displayRewardsInPortfolio, externalRewardsNetwork, networks])

  const [visiblePortfolioNetworks, visiblePortfolioNetworkIds] =
    React.useMemo(() => {
      const visibleNetworks = networksList.filter(
        (network) =>
          !filteredOutPortfolioNetworkKeys.includes(
            networkEntityAdapter.selectId(network).toString()
          )
      )
      return [
        visibleNetworks,
        visibleNetworks.map(networkEntityAdapter.selectId)
      ]
    }, [networksList, filteredOutPortfolioNetworkKeys])

  // render
  return {
    visiblePortfolioNetworkIds,
    visiblePortfolioNetworks,
    filteredOutPortfolioNetworkKeys
  }
}
