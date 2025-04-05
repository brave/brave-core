// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../../../../common/locale'

// Types
import { BraveWallet } from '../../../../../../constants/types'

// Components
import { CreateNetworkIcon } from '../../../../../shared/create-network-icon'

// Styled Components
import {
  NetworkButton,
  NetworkName,
  ActiveIndicator
} from './change_network_button.style'
import { Row } from '../../../../../shared/style'

interface Props {
  network: BraveWallet.NetworkInfo
  isActiveNetwork: boolean
  onClick: () => void
}

export const ChangeNetworkButton = (props: Props) => {
  const { network, isActiveNetwork, onClick } = props

  return (
    <NetworkButton
      onClick={onClick}
      data-test-chain-id={'chain-' + network.chainId}
    >
      <Row width='unset'>
        <CreateNetworkIcon
          network={network}
          marginRight={12}
          size='big'
        />
        <NetworkName>{network.chainName}</NetworkName>
      </Row>
      {isActiveNetwork && (
        <ActiveIndicator>{getLocale('braveWalletActive')}</ActiveIndicator>
      )}
    </NetworkButton>
  )
}
