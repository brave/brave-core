// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import {
  reduceNetworkDisplayName //
} from '../../../utils/network-utils'

// Options
import {
  AllNetworksOption //
} from '../../../options/network-filter-options'

// Components
import {
  CreateNetworkIcon //
} from '../create-network-icon'

// Styled Components
import { DropdownFilter, DropdownOption } from './shared_dropdown.styles'
import { Row } from '../style'

interface Props {
  networks: BraveWallet.NetworkInfo[]
  selectedNetwork: BraveWallet.NetworkInfo
  showAllNetworksOption?: boolean
  onSelectNetwork: (chainId: string) => void
  checkIsNetworkOptionDisabled?: (network: BraveWallet.NetworkInfo) => boolean
}

export const NetworksDropdown = (props: Props) => {
  const {
    networks,
    selectedNetwork,
    showAllNetworksOption,
    onSelectNetwork,
    checkIsNetworkOptionDisabled
  } = props

  return (
    <DropdownFilter
      onChange={(e) => onSelectNetwork(e.value ?? '')}
      value={selectedNetwork.chainId}
    >
      <Row
        slot='value'
        justifyContent='flex-start'
      >
        {selectedNetwork.chainId !== AllNetworksOption.chainId && (
          <CreateNetworkIcon
            network={selectedNetwork}
            marginRight={8}
            size='medium'
          />
        )}
        {selectedNetwork.chainId === AllNetworksOption.chainId
          ? selectedNetwork.chainName
          : reduceNetworkDisplayName(selectedNetwork.chainName)}
      </Row>
      {showAllNetworksOption && (
        <leo-option value={AllNetworksOption.chainId}>
          <DropdownOption justifyContent='space-between'>
            <Row width='unset'>{AllNetworksOption.chainName}</Row>
            {selectedNetwork.chainId === AllNetworksOption.chainId && (
              <Icon name='check-normal' />
            )}
          </DropdownOption>
        </leo-option>
      )}
      {networks.map((network) => (
        <leo-option
          value={network.chainId}
          key={network.chainId}
        >
          <DropdownOption
            justifyContent='space-between'
            isDisabled={
              checkIsNetworkOptionDisabled
                ? checkIsNetworkOptionDisabled(network)
                : false
            }
          >
            <Row width='unset'>
              {network.chainId !== AllNetworksOption.chainId && (
                <CreateNetworkIcon
                  network={network}
                  marginRight={8}
                  size='medium'
                />
              )}
              {network.chainName}
            </Row>
            {selectedNetwork.chainId === network.chainId && (
              <Icon name='check-normal' />
            )}
          </DropdownOption>
        </leo-option>
      ))}
    </DropdownFilter>
  )
}
