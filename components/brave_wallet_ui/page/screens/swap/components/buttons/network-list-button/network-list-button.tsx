// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../../../../constants/types'

// Components
import {
  CreateNetworkIcon //
} from '../../../../../../components/shared/create-network-icon'

// Styled Components
import { Button } from './network-list-button.style'
import { Text, Row } from '../../shared-swap.styles'

interface Props {
  onClick: (network: BraveWallet.NetworkInfo) => void
  network: BraveWallet.NetworkInfo
  isHeader?: boolean
}

export const NetworkListButton = (props: Props) => {
  const { network, isHeader, onClick } = props

  // Methods
  const onSelectNetwork = React.useCallback(() => {
    onClick(network)
  }, [network, onClick])

  return (
    <Button
      onClick={onSelectNetwork}
      isHeader={isHeader}
    >
      <Row>
        <CreateNetworkIcon
          size='big'
          marginRight={8}
          network={network}
        />
        <Text
          isBold={true}
          textSize='14px'
        >
          {network.chainName}
        </Text>
      </Row>
    </Button>
  )
}
