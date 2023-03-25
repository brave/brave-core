// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { BraveWallet } from '../../../constants/types'

// hooks
import { useGetVisibleNetworksQuery } from '../../../common/slices/api.slice'

// components
import SelectNetworkItem from '../select-network-item'

interface Props {
  onSelectCustomNetwork?: (network: BraveWallet.NetworkInfo) => void
  selectedNetwork: BraveWallet.NetworkInfo | undefined
  customNetwork?: BraveWallet.NetworkInfo
}

export function SelectNetwork ({
  onSelectCustomNetwork,
  selectedNetwork,
  customNetwork
}: Props) {
  // queries
  const { data: networks = [] } = useGetVisibleNetworksQuery()

  // memos
  const networksList = React.useMemo(() => {
    if (customNetwork) {
      return [customNetwork, ...networks]
    }
    return networks
  }, [networks, customNetwork])

  // render
  return (
    <>
      {networksList.map((network) =>
        <SelectNetworkItem
          selectedNetwork={selectedNetwork}
          key={`${network.chainId}-${network.coin}`}
          network={network}
          onSelectCustomNetwork={onSelectCustomNetwork}
        />
      )}
    </>
  )
}

export default SelectNetwork
