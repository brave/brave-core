// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../../../../common/locale'

// Hooks
import {
  useGetSwapSupportedNetworksQuery //
} from '../../../../../../common/slices/api.slice'

// Types
import { BraveWallet } from '../../../../../../constants/types'

// Components
import {
  NetworkListButton //
} from '../../buttons/network-list-button/network-list-button'

// Styled Components
import { HeaderRow, SelectorBox } from './network-selector.style'
import {
  Text,
  // VerticalDivider,
  VerticalSpacer,
  IconButton,
  Icon
} from '../../shared-swap.styles'

interface Props {
  onSelectNetwork: (network: BraveWallet.NetworkInfo) => void
  onClose?: () => void
  isHeader?: boolean
}

export const NetworkSelector = (props: Props) => {
  const { onSelectNetwork, onClose, isHeader } = props

  const { data: supportedNetworks } = useGetSwapSupportedNetworksQuery()

  return (
    <SelectorBox isHeader={isHeader}>
      {/*
       * TODO(onyb): enable or remove this.
       *
       * Disabling this until we support fee estimates
       **/}
      {/* <VerticalSpacer size={12} />
      <Row horizontalPadding={12} rowWidth='full'>
        <Text textSize='12px' textColor='text03' isBold={false}>
          {getLocale('braveSwapName')}
        </Text>
        <Text textSize='12px' textColor='text03' isBold={false}>
          {getLocale('braveSwapNetworkFee')}
        </Text>
      </Row>
      <VerticalSpacer size={8} />
      <VerticalDivider /> */}
      <HeaderRow
        isHeader={isHeader}
        rowWidth='full'
        horizontalPadding={20}
        verticalPadding={12}
      >
        <Text
          textSize='20px'
          textColor='text01'
          isBold={true}
        >
          {getLocale('braveSwapChangeNetwork')}
        </Text>
        <IconButton onClick={onClose}>
          <Icon
            size={24}
            name='close'
          />
        </IconButton>
      </HeaderRow>
      <VerticalSpacer size={4} />
      {supportedNetworks?.map((network) => (
        <NetworkListButton
          key={network.chainId}
          onClick={onSelectNetwork}
          network={network}
          isHeader={isHeader}
        />
      ))}
    </SelectorBox>
  )
}
