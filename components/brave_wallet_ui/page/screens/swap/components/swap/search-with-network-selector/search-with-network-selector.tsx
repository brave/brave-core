// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../../../../common/locale'
import { reduceNetworkDisplayName } from '../../../../../../utils/network-utils'

// Types
import { BraveWallet } from '../../../../../../constants/types'

// Components
import {
  SelectTokenOrNetworkButton //
} from '../../buttons/select-token-or-network/select-token-or-network'
import { SearchInput } from '../../inputs/search-input/search-input'
import { NetworkSelector } from '../network-selector/network-selector'

// Styled Components
import { Wrapper, SelectorWrapper } from './search-with-network-selector.style'
import {
  HorizontalDivider,
  HiddenResponsiveRow
} from '../../shared-swap.styles'

interface Props {
  onSearchChanged: (value: string) => void
  searchValue: string
  networkSelectorDisabled: boolean
  selectedNetwork: BraveWallet.NetworkInfo | undefined
  setSelectedNetwork: (network: BraveWallet.NetworkInfo) => void
}

export const SearchWithNetworkSelector = (props: Props) => {
  const {
    onSearchChanged,
    searchValue,
    networkSelectorDisabled,
    selectedNetwork,
    setSelectedNetwork
  } = props

  // State
  const [showNetworkSelector, setShowNetworkSelector] =
    React.useState<boolean>(false)

  const onSelectNetwork = React.useCallback(
    async (network: BraveWallet.NetworkInfo) => {
      setSelectedNetwork(network)
      setShowNetworkSelector(false)
    },
    [setSelectedNetwork]
  )

  return (
    <Wrapper>
      <SearchInput
        placeholder={getLocale('braveSwapSearchToken')}
        autoFocus={false}
        onChange={onSearchChanged}
        value={searchValue}
      />
      <HiddenResponsiveRow maxWidth={570}>
        <HorizontalDivider
          marginRight={8}
          height={24}
        />
        <SelectorWrapper>
          <SelectTokenOrNetworkButton
            network={selectedNetwork}
            onClick={() => setShowNetworkSelector((prev) => !prev)}
            text={reduceNetworkDisplayName(selectedNetwork?.chainName)}
            buttonSize='small'
            disabled={networkSelectorDisabled}
            iconType='network'
          />
          {showNetworkSelector && (
            <NetworkSelector onSelectNetwork={onSelectNetwork} />
          )}
        </SelectorWrapper>
      </HiddenResponsiveRow>
    </Wrapper>
  )
}
