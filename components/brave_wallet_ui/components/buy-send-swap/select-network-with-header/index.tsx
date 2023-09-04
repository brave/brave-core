// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import {
  useGetSelectedChainQuery,
  useSetNetworkMutation
} from '../../../common/slices/api.slice'
import { getLocale } from '../../../../common/locale'

// Components
import { SelectNetwork } from '../../shared/select-network/index'
import Header from '../select-header'

// Styled Components
import {
  SelectWrapper,
  SelectScrollContainer
} from '../shared-styles'

export interface Props {
  hasAddButton?: boolean
  onBack: () => void
  onAddNetwork: () => void
  onNoAccountForNetwork: (network: BraveWallet.NetworkInfo) => void
}

export const SelectNetworkWithHeader = ({
  onBack,
  hasAddButton,
  onAddNetwork,
  onNoAccountForNetwork
}: Props) => {
  // queries & mutations
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const [setNetwork] = useSetNetworkMutation()

  // methods
  const onSelectCustomNetwork = React.useCallback(
    async (network: BraveWallet.NetworkInfo) => {
      const { needsAccountForNetwork } = await setNetwork({
        chainId: network.chainId,
        coin: network.coin
      }).unwrap()
      if (needsAccountForNetwork) {
        onNoAccountForNetwork(network)
      } else {
        onBack()
      }
    },
    [onBack, setNetwork, onNoAccountForNetwork]
  )

  // render
  return (
    <SelectWrapper>
      <Header
        title={getLocale('braveWalletSelectNetwork')}
        onBack={onBack}
        hasAddButton={hasAddButton}
        onClickAdd={onAddNetwork}
      />
      <SelectScrollContainer>
        <SelectNetwork
          onSelectCustomNetwork={onSelectCustomNetwork}
          selectedNetwork={selectedNetwork}
        />
      </SelectScrollContainer>
    </SelectWrapper>
  )
}

export default SelectNetworkWithHeader
