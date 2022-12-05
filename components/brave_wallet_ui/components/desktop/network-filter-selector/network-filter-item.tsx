// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Components
import { CreateNetworkIcon } from '../../shared'

// Options
import { AllNetworksOption } from '../../../options/network-filter-options'

// Styled Components
import { NetworkItemButton, NetworkName, LeftSide, NetworkItemWrapper, BigCheckMark } from './style'

export interface Props {
  children?: React.ReactNode
  selectedNetwork: BraveWallet.NetworkInfo
  network: BraveWallet.NetworkInfo
  onSelectNetwork: (network?: BraveWallet.NetworkInfo) => void
}

function NetworkFilterItem (props: Props) {
  const { network, onSelectNetwork, selectedNetwork } = props

  const onClickSelectNetwork = () => {
    onSelectNetwork(network)
  }

  return (
    <NetworkItemWrapper>
      <NetworkItemButton onClick={onClickSelectNetwork}>
        <LeftSide>
          {network.chainId !== AllNetworksOption.chainId && (
            <CreateNetworkIcon network={network} marginRight={14} size='big' />
          )}
          <NetworkName>{network.chainName}</NetworkName>
        </LeftSide>
        {network.chainId === selectedNetwork.chainId &&
          network.symbol.toLowerCase() === selectedNetwork.symbol.toLowerCase() && <BigCheckMark />}
      </NetworkItemButton>
    </NetworkItemWrapper>
  )
}

export default NetworkFilterItem
