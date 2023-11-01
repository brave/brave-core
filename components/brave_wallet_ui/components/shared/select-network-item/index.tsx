// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Constants
import { BraveWallet } from '../../../constants/types'
import { AllNetworksOption } from '../../../options/network-filter-options'

// Components
import { CreateNetworkIcon } from '../create-network-icon/index'

// Styled Components
import { StyledWrapper, NetworkName, LeftSide, BigCheckMark } from './style'

export interface Props {
  selectedNetwork?: BraveWallet.NetworkInfo | null
  network: BraveWallet.NetworkInfo
  onSelectCustomNetwork: (network: BraveWallet.NetworkInfo) => void
}

export function SelectNetworkItem(props: Props) {
  const { network, selectedNetwork, onSelectCustomNetwork } = props

  // methods
  const onSelectNetwork = React.useCallback(async () => {
    onSelectCustomNetwork(network)
  }, [onSelectCustomNetwork, network])

  // render
  return (
    <StyledWrapper
      onClick={onSelectNetwork}
      data-test-chain-id={'chain-' + network.chainId}
    >
      <LeftSide>
        {network.chainId !== AllNetworksOption.chainId && (
          <CreateNetworkIcon
            network={network}
            marginRight={14}
          />
        )}
        <NetworkName>{network.chainName}</NetworkName>
      </LeftSide>
      {selectedNetwork?.chainId === network.chainId &&
        selectedNetwork?.coin === network.coin && <BigCheckMark />}
    </StyledWrapper>
  )
}

export default SelectNetworkItem
