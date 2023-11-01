// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Checkbox from '@brave/leo/react/checkbox'

// Types
import { BraveWallet } from '../../../../../constants/types'

// Components
import {
  CreateNetworkIcon //
} from '../../../../shared/create-network-icon/index'

// Utils
import {
  networkEntityAdapter //
} from '../../../../../common/slices/entities/network.entity'

// Styled Components
import {
  CheckboxText,
  CheckboxRow,
  CheckboxWrapper,
  Description
} from './filter-components.style'
import { Row } from '../../../../shared/style'

interface Props {
  networks: BraveWallet.NetworkInfo[]
  title: string
  marginBottom: number
  isNetworkFilteredOut: (key: string) => boolean
  onCheckNetwork: (key: string) => void
}

export const NetworkCheckboxes = (props: Props) => {
  const {
    networks,
    title,
    marginBottom,
    isNetworkFilteredOut,
    onCheckNetwork
  } = props

  return (
    <>
      <Row
        marginBottom={8}
        justifyContent='flex-start'
      >
        <Description
          textSize='12px'
          isBold={true}
        >
          {title}
        </Description>
      </Row>
      <CheckboxRow
        justifyContent='space-between'
        marginBottom={marginBottom}
      >
        {networks.map((network) => (
          <CheckboxWrapper
            width='unset'
            justifyContent='flex-start'
            marginBottom={16}
            key={networkEntityAdapter.selectId(network).toString()}
          >
            <Checkbox
              checked={
                !isNetworkFilteredOut(
                  networkEntityAdapter.selectId(network).toString()
                )
              }
              onChange={() =>
                onCheckNetwork(
                  networkEntityAdapter.selectId(network).toString()
                )
              }
            >
              <CreateNetworkIcon
                network={network}
                marginRight={0}
                size='big'
              />
              <CheckboxText
                textSize='14px'
                isBold={false}
              >
                {network.chainName}
              </CheckboxText>
            </Checkbox>
          </CheckboxWrapper>
        ))}
      </CheckboxRow>
    </>
  )
}
