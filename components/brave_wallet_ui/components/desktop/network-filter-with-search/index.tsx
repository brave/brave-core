// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Options
import { AllNetworksOption } from '../../../options/network-filter-options'

// Components
import SelectNetworkDropdown from '../select-network-dropdown'
import SearchBar from '../../shared/search-bar'

// Styled Components
import {
  StyledWrapper,
  HorizontalDivider
} from './style'

interface Props {
  selectedNetwork: BraveWallet.NetworkInfo
  showNetworkDropDown: boolean
  onClick: () => void
  onSelectNetwork?: (network: BraveWallet.NetworkInfo) => void
  searchPlaceholder: string
  searchAction?: (event: any) => void | undefined
  searchAutoFocus?: boolean
  searchValue?: string
}

export const NetworkFilterWithSearch = (props: Props) => {
  const {
    selectedNetwork,
    onClick,
    showNetworkDropDown,
    onSelectNetwork,
    searchPlaceholder,
    searchAction,
    searchAutoFocus,
    searchValue
  } = props

  return (
    <StyledWrapper>
      <SearchBar
        placeholder={searchPlaceholder}
        action={searchAction}
        autoFocus={searchAutoFocus}
        value={searchValue}
        useWithFilter={true}
      />
      <HorizontalDivider />
      <SelectNetworkDropdown
        onSelectCustomNetwork={onSelectNetwork}
        selectedNetwork={selectedNetwork}
        onClick={onClick}
        showNetworkDropDown={showNetworkDropDown}
        useWithSearch={true}
        customNetwork={AllNetworksOption}
      />
    </StyledWrapper>
  )
}

export default NetworkFilterWithSearch
