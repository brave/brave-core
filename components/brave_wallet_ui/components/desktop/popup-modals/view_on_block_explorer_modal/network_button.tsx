// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Types
import { BraveWallet } from '../../../../constants/types'

// Custom hooks
import { useExplorer } from '../../../../common/hooks/explorer'

// Utils
import { getLocale } from '../../../../../common/locale'

// Components
import { CreateNetworkIcon } from '../../../shared/create-network-icon'

// Styles
import { Button, LaunchIcon } from './view_on_block_explorer_modal.style'
import { Text, Row } from '../../../shared/style'

interface Props {
  network: BraveWallet.NetworkInfo
  address: string
}

export const NetworkButton = (props: Props) => {
  const { network, address } = props

  // Hooks
  const onClickViewOnBlockExplorer = useExplorer(network)

  return (
    <Button onClick={onClickViewOnBlockExplorer('address', address)}>
      <Row width='unset'>
        <CreateNetworkIcon
          network={network}
          marginRight={12}
          size='big'
        />
        <Text
          isBold={true}
          textColor='primary'
          textSize='14px'
        >
          {getLocale('braveWalletNetworkExplorer').replace(
            '$1',
            network.chainName
          )}
        </Text>
      </Row>
      <LaunchIcon />
    </Button>
  )
}
