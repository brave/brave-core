// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

import SelectNetworkItem from '../select-network-item'
import { BraveWallet, WalletState } from '../../../constants/types'

interface Props {
  onSelectCustomNetwork?: (network: BraveWallet.NetworkInfo) => void
  selectedNetwork: BraveWallet.NetworkInfo | undefined
  customNetwork?: BraveWallet.NetworkInfo
}

function SelectNetwork ({
  onSelectCustomNetwork,
  selectedNetwork,
  customNetwork
}: Props) {
  // redux
  const networks = useSelector(({ wallet }: { wallet: WalletState }) => wallet.networkList)

  const networksList = React.useMemo(() => {
    if (customNetwork) {
      return [customNetwork, ...networks]
    }
    return networks
  }, [networks, customNetwork])

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
