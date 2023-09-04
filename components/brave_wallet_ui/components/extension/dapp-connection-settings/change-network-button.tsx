// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../common/locale'

// Queries
import {
  useGetSelectedChainQuery
} from '../../../common/slices/api.slice'

// Types
import {
  BraveWallet
} from '../../../constants/types'

// Components
import {
  CreateNetworkIcon
} from '../../shared/create-network-icon'

// Styled Components
import {
  NetworkButton,
  NetworkName,
  ActiveIndicator
} from './dapp-connection-settings.style'
import {
  Row
} from '../../shared/style'

interface Props {
  network: BraveWallet.NetworkInfo
  onSelectNetwork: (network: BraveWallet.NetworkInfo) => void
}

export const ChangeNetworkButton = (props: Props) => {
  const {
    network,
    onSelectNetwork
  } = props

  // Queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()

  // Methods
  const onClick = React.useCallback(() => {
    onSelectNetwork(network)
  }, [onSelectNetwork, network])

  return (
    <NetworkButton
      onClick={onClick}
      data-test-chain-id={'chain-' + network.chainId}
    >
      <Row
        width='unset'
      >
        <CreateNetworkIcon
          network={network}
          marginRight={8}
          size='big'
        />
        <NetworkName>
          {network.chainName}
        </NetworkName>
      </Row>
      {selectedNetwork?.chainId === network.chainId &&
        <ActiveIndicator>
          {getLocale('braveWalletActive')}
        </ActiveIndicator>
      }
    </NetworkButton>
  )
}
