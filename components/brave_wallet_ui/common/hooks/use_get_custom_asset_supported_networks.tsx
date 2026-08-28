// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useGetVisibleNetworksQuery } from '../slices/api.slice'
import {
  BraveWallet,
  CustomAssetSupportedCoinTypes,
  PolkadotAssetHubChainIds,
} from '../../constants/types'

export function useGetCustomAssetSupportedNetworks(
  supportedCoinTypes: BraveWallet.CoinType[] = CustomAssetSupportedCoinTypes,
) {
  const { data: networkList = [] } = useGetVisibleNetworksQuery()

  return React.useMemo(
    () =>
      networkList.filter(
        (network) =>
          supportedCoinTypes.includes(network.coin)
          && (network.coin !== BraveWallet.CoinType.DOT
            || PolkadotAssetHubChainIds.includes(network.chainId)),
      ),
    [networkList, supportedCoinTypes],
  )
}
