// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { reduceNetworkDisplayName } from '../../../utils/network-utils'

// Options
import { AllNetworksOption } from '../../../options/network-filter-options'

// Types
import { BraveWallet } from '../../../constants/types'

// Components
import { CreateNetworkIcon } from '../../shared/create-network-icon/index'
import { SelectNetwork } from '../../shared/select-network/index'

// Styled Components
import {
  StyledWrapper,
  DropDown,
  NetworkButton,
  DropDownIcon,
  LeftSide,
  NetworkText
} from './style'
import { getLocale } from '../../../../common/locale'

interface Props {
  selectedNetwork: BraveWallet.NetworkInfo | undefined
  showNetworkDropDown: boolean
  onClick: () => void
  onSelectCustomNetwork: (network: BraveWallet.NetworkInfo) => void
  useWithSearch?: boolean
  customNetwork?: BraveWallet.NetworkInfo | undefined
  networkListSubset?: BraveWallet.NetworkInfo[]
  disabled?: boolean
  reduceDisplayName?: boolean
}

export const SelectNetworkDropdown = (props: Props) => {
  const {
    selectedNetwork,
    onClick,
    showNetworkDropDown,
    onSelectCustomNetwork,
    useWithSearch,
    customNetwork,
    networkListSubset,
    disabled,
    reduceDisplayName
  } = props

  const networkDisplayName = selectedNetwork?.chainName
    ? reduceDisplayName && selectedNetwork.chainId !== AllNetworksOption.chainId
      ? reduceNetworkDisplayName(selectedNetwork.chainName)
      : selectedNetwork.chainName
    : getLocale('braveWalletSelectNetwork')

  return (
    <StyledWrapper useWithSearch={useWithSearch}>
      <NetworkButton
        onClick={onClick}
        useWithSearch={useWithSearch}
        disabled={disabled}
      >
        <LeftSide>
          {selectedNetwork &&
            selectedNetwork.chainId !== AllNetworksOption.chainId && (
              <CreateNetworkIcon
                network={selectedNetwork}
                marginRight={14}
                size='big'
              />
            )}
          <NetworkText>{networkDisplayName}</NetworkText>
        </LeftSide>
        <DropDownIcon isOpen={showNetworkDropDown} />
      </NetworkButton>
      {showNetworkDropDown && (
        <DropDown useWithSearch={useWithSearch}>
          <SelectNetwork
            onSelectCustomNetwork={onSelectCustomNetwork}
            selectedNetwork={selectedNetwork}
            customNetwork={customNetwork}
            networkListSubset={networkListSubset}
          />
        </DropDown>
      )}
    </StyledWrapper>
  )
}

export default SelectNetworkDropdown
