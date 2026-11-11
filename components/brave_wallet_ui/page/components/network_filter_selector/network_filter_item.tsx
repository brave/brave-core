// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '$wallet/constants/types'

// Components
import { CreateNetworkIcon } from '$wallet/components/shared/create-network-icon/index'

// Options
import { AllNetworksOption } from '$wallet/options/network-filter-options'

// Styled Components
import {
  NetworkItemButton,
  LeftSide,
  NetworkItemWrapper,
  BigCheckMark,
} from './network_filter_selector.style'
import { Text } from '$wallet/components/shared/style'

export interface Props {
  isSelected: boolean
  network: BraveWallet.NetworkInfo
  onSelectNetwork: (network?: BraveWallet.NetworkInfo) => void
}

export const NetworkFilterItem = (props: Props) => {
  const { network, onSelectNetwork, isSelected } = props

  const onClickSelectNetwork = () => {
    onSelectNetwork(network)
  }

  return (
    <NetworkItemWrapper>
      <NetworkItemButton onClick={onClickSelectNetwork}>
        <LeftSide>
          {network.chainId !== AllNetworksOption.chainId && (
            <CreateNetworkIcon
              network={network}
              marginRight={14}
              size='big'
            />
          )}
          <Text
            textColor='primary'
            variant='default.regular'
            textAlign='left'
          >
            {network.chainName}
          </Text>
        </LeftSide>
        {isSelected ? <BigCheckMark /> : null}
      </NetworkItemButton>
    </NetworkItemWrapper>
  )
}
